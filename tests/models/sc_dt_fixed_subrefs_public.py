"""Compare fixed-point range proxies with an independent native reference."""

import gc
from pathlib import Path
import sys
import weakref

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_dt_fixed_subrefs_probe.h"))
PROBE = cppyy.gbl.sc_dt_fixed_subrefs_probe


def flatten(value):
    """Replace native dump line endings so each proxy dump occupies one row.

    Args:
        value: Text copied from a native proxy stream operation.

    Returns:
        Text with carriage returns and newlines replaced by vertical bars.
    """
    return str(value).replace("\r", "|").replace("\n", "|")


def emit(*fields):
    """Emit one tab-separated semantic observation.

    Args:
        *fields: Values to place in an FXSUB record.
    """
    print("FXSUB\t" + "\t".join(str(field) for field in fields))


def read_view(family, owner_name, label, high, low, view):
    """Emit every read-only proxy method and native conversion result.

    Args:
        family: Read-only proxy family label.
        owner_name: Parent number label used by the reference rows.
        label: Range selection label.
        high: Inclusive public high index, or -1 for the whole word.
        low: Inclusive public low index, or -1 for the whole word.
        view: Live native range proxy whose owner outlives this operation.
    """
    print_stream = cppyy.gbl.std.ostringstream()
    dump_stream = cppyy.gbl.std.ostringstream()
    view.print(print_stream)
    view.dump(dump_stream)
    bits = PROBE.to_bit_vector(view) if family == "fixed_r" else PROBE.to_fast_bit_vector(view)
    emit(
        "view",
        family,
        owner_name,
        label,
        high,
        low,
        view.length(),
        str(view.to_string()),
        str(view.to_string(dt.SC_BIN, True)),
        str(view.to_string(dt.SC_DEC)),
        int(view.to_int()),
        int(view.to_uint()),
        int(view.to_long()),
        int(view.to_ulong()),
        int(view.to_int64()),
        int(view.to_uint64()),
        int(view.and_reduce()),
        int(view.nand_reduce()),
        int(view.or_reduce()),
        int(view.nor_reduce()),
        int(view.xor_reduce()),
        int(view.xnor_reduce()),
        str(bits.to_string(dt.SC_BIN, True)),
        str(print_stream.str()),
        flatten(dump_stream.str()),
    )
    emit("signed_compat", family, owner_name, label, int(view.to_signed()), int(view.to_unsigned()))


def selection(owner, path, high=None, low=None):
    """Select a mutable range through the named public access path.

    Args:
        owner: Fixed-point parent retained by the returned view.
        path: ``range`` or ``call`` to select the overload family.
        high: Optional inclusive high index for a two-index view.
        low: Optional inclusive low index for a two-index view.

    Returns:
        A mutable range proxy that borrows owner and carries its Python parent.
    """
    accessor = owner.range if path == "range" else owner
    return accessor() if high is None else accessor(high, low)


def exercise_reads(family, owner_name, owner, ranges, is_fast):
    """Use native const helpers to inspect normal, reversed and fractional views.

    Args:
        family: The fixed or fast read-only reference family.
        owner_name: Label for the configured native parent.
        owner: Parent fixed-point value held through every proxy operation.
        ranges: Named inclusive range pairs to inspect.
        is_fast: Choose the const fast helper when true.
    """
    for label, high, low in ranges:
        if label == "call_const":
            view = PROBE.fast_const_call(owner, high, low) if is_fast else PROBE.fixed_const_call(owner, high, low)
        else:
            view = PROBE.fast_const_range(owner, high, low) if is_fast else PROBE.fixed_const_range(owner, high, low)
        read_view(family, owner_name, label, high, low, view)

    whole_call = PROBE.fast_const_call_whole(owner) if is_fast else PROBE.fixed_const_call_whole(owner)
    read_view(family, owner_name, "whole_call", -1, -1, whole_call)
    whole_range = PROBE.fast_const_whole(owner) if is_fast else PROBE.fixed_const_whole(owner)
    read_view(family, owner_name, "whole_range", -1, -1, whole_range)


def exercise_accessor_copies(owner_name, owner):
    """Compare `.range`, `__call__`, and whole-word access with native semantics.

    Args:
        owner_name: Transcript label for the mutable owner.
        owner: Live parent held while all copied native proxies are used.
    """
    high, low = 11, 4
    range_view = owner.range(high, low)
    range_copy = type(range_view)(range_view)
    range_copy._pysystemc_parent = owner
    owner.range(high, low).__assign__("10101010")
    emit("copy", owner_name, "range", high, low, range_copy.to_string(dt.SC_BIN, True), str(owner.to_bin()))

    call_view = owner(high, low)
    call_copy = type(call_view)(call_view)
    call_copy._pysystemc_parent = owner
    owner(high, low).__assign__("01010101")
    emit("copy", owner_name, "call", high, low, call_copy.to_string(dt.SC_BIN, True), str(owner.to_bin()))

    for name, proxy in (
        ("range_args", owner.range(high, low)),
        ("call_args", owner(high, low)),
        ("range_whole", owner.range()),
        ("call_whole", owner()),
    ):
        emit(
            "access",
            owner_name,
            name,
            high if name.endswith("args") else -1,
            low if name.endswith("args") else -1,
            proxy.length(),
            str(proxy.to_string(dt.SC_BIN, True)),
        )


def exercise_assignments(target_name, target_type):
    """Exercise the 17 declared assignment source signatures per mutable family.

    Args:
        target_name: Mutable family label for transcript rows.
        target_type: Parameterized arbitrary or fast fixed-point parent type.
    """
    fixed_source = dt.sc_fixed[16, 8](-5.25)
    fast_source = dt.sc_fixed_fast[16, 8](-5.25)
    bit_vector = dt.sc_bv_base("10100101")
    logic_vector = dt.sc_lv_base("01011010")
    signed_value = dt.sc_signed(8)
    signed_value.__assign__(-91)
    unsigned_value = dt.sc_unsigned(8)
    unsigned_value.__assign__(165)
    signed_base = dt.sc_int_base(-91, 8)
    unsigned_base = dt.sc_uint_base(165, 8)
    sources = (
        ("fixed_subref", fixed_source.range(11, 4), None),
        ("fast_subref", fast_source.range(11, 4), None),
        ("sc_bv_base", bit_vector, None),
        ("sc_lv_base", logic_vector, None),
        ("char_pointer", "01101001", "const char*"),
        ("bool_pointer", PROBE.bool_assignment_pattern(), "const bool*"),
        ("sc_signed", signed_value, None),
        ("sc_unsigned", unsigned_value, None),
        ("sc_int_base", signed_base, None),
        ("sc_uint_base", unsigned_base, None),
        ("int64", -91, "sc_dt::int64"),
        ("uint64", 165, "sc_dt::uint64"),
        ("int", -91, "int"),
        ("unsigned_int", 165, "unsigned int"),
        ("long", -91, "long"),
        ("unsigned_long", 165, "unsigned long"),
        ("char", "A", "char"),
    )
    for source_name, source, signature in sources:
        target = target_type(0)
        proxy = target.range(11, 4)
        if signature:
            proxy.__assign__.__overload__(signature)(source)
        else:
            proxy.__assign__(source)
        emit(
            "assign",
            target_name,
            "fixed16" if target_name == "fixed" else "fast16",
            source_name,
            str(proxy.to_string(dt.SC_BIN, True)),
            str(target.to_bin()),
        )


def exercise_compounds(target_name, target_type):
    """Exercise all 12 declared compound sources per mutable proxy family.

    Args:
        target_name: Mutable family label for transcript rows.
        target_type: Parameterized arbitrary or fast fixed-point parent type.
    """
    fixed_source = dt.sc_fixed[16, 8](-5.25)
    fast_source = dt.sc_fixed_fast[16, 8](-5.25)
    bit_vector = dt.sc_bv_base("10100101")
    logic_vector = dt.sc_lv_base("01011010")
    sources = (
        ("fixed_subref", fixed_source.range(11, 4)),
        ("fast_subref", fast_source.range(11, 4)),
        ("sc_bv_base", bit_vector),
        ("sc_lv_base", logic_vector),
    )
    for operator_name, method_name in (("and", "__iand__"), ("or", "__ior__"), ("xor", "__ixor__")):
        for source_name, source in sources:
            target = target_type(-2.75)
            proxy = target.range(11, 4)
            getattr(proxy, method_name)(source)
            emit(
                "compound",
                target_name,
                operator_name,
                source_name,
                str(proxy.to_string(dt.SC_BIN, True)),
                str(target.to_bin()),
            )


def exercise_parent_lifelines(number_type, expected_type):
    """Verify parent retention for ranged and whole views through both entry paths.

    Args:
        number_type: Fixed or fast specialization used for the owner.
        expected_type: Exact mutable subreference C++ class name.
    """
    selectors = (
        ("range_args", lambda owner: owner.range(11, 4)),
        ("call_args", lambda owner: owner(11, 4)),
        ("range_whole", lambda owner: owner.range()),
        ("call_whole", lambda owner: owner()),
    )
    for label, select in selectors:
        parent = number_type(0)
        parent_ref = weakref.ref(parent)
        proxy = select(parent)
        assert type(proxy).__cpp_name__ == expected_type
        assert proxy._pysystemc_parent is parent
        del parent
        gc.collect()
        assert parent_ref() is not None
        proxy.__assign__(165)
        owner = proxy._pysystemc_parent
        assert proxy.to_uint() == 165
        assert owner is parent_ref()


def exercise_vector_copy_conversions():
    """Copy both read-only range types into base and arbitrary-width vectors.

    The resulting vector objects own their storage independently of the source
    range and fixed-point number.
    """
    for family, number_type, const_range in (
        ("fixed_r", dt.sc_fixed[16, 8], PROBE.fixed_const_range),
        ("fast_r", dt.sc_fixed_fast[16, 8], PROBE.fast_const_range),
    ):
        parent = number_type("-5.25")
        parent_ref = weakref.ref(parent)
        view = const_range(parent, 11, 4)
        base_copy = dt.sc_bv_base(view)
        typed_copy = dt.sc_bv[view.length()](view)
        base_bits = str(base_copy.to_string(dt.SC_BIN, True))
        typed_bits = str(typed_copy.to_string(dt.SC_BIN, True))
        del view, parent
        gc.collect()
        assert parent_ref() is None
        assert str(base_copy.to_string(dt.SC_BIN, True)) == base_bits
        assert str(typed_copy.to_string(dt.SC_BIN, True)) == typed_bits
        emit("vector_copy", family, base_bits, typed_bits, base_copy.length(), typed_copy.length())


def run_error_mode(mode):
    """Run an isolated native bounds report with SystemC's throw action.

    Args:
        mode: ``negative-fixed`` or ``upper-fast``.
    """
    previous = sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_THROW)
    try:
        if mode == "negative-fixed":
            dt.sc_fixed[16, 8](1).range(-1, 0)
        elif mode == "upper-fast":
            dt.sc_fixed_fast[16, 8](1).range(16, 0)
        else:
            raise ValueError(f"unknown fixed subref mode: {mode}")
    except sc.sc_report as report:
        emit("report", mode, report.get_msg_type(), report.get_msg())
    finally:
        sc.sc_report_handler.set_actions(sc.SC_ERROR, previous)


if __name__ == "__main__" and len(sys.argv) > 1:
    run_error_mode(sys.argv[1])
else:
    fixed = dt.sc_fixed[137, 69](-13.375)
    fast = dt.sc_fixed_fast[64, 32](-13.375)
    fast_wide = dt.sc_fixed_fast[137, 100]("9007199254740992.25")
    emit("width", "fixed137", fixed.wl(), fixed.iwl(), str(fixed.to_bin()))
    emit("width", "fast64", fast.wl(), fast.iwl(), str(fast.to_bin()))
    emit("width", "fast137", fast_wide.wl(), fast_wide.iwl(), str(fast_wide.to_bin()))

    fixed_ranges = (("normal", 103, 96), ("reversed", 96, 103), ("fractional", 7, 0), ("boundary", 72, 64))
    fast_ranges = (("normal", 48, 41), ("reversed", 41, 48), ("fractional", 7, 0), ("boundary", 34, 28))
    fast_wide_ranges = (("normal", 85, 78), ("reversed", 78, 85), ("fractional", 30, 23), ("boundary", 40, 34))
    exercise_reads("fixed_r", "fixed137", fixed, fixed_ranges, False)
    exercise_reads("fast_r", "fast64", fast, fast_ranges, True)
    exercise_reads("fast_r", "fast137", fast_wide, fast_wide_ranges, True)

    exercise_accessor_copies("fixed137", fixed)
    exercise_accessor_copies("fast64", fast)
    exercise_accessor_copies("fast137", fast_wide)

    exercise_assignments("fixed", dt.sc_fixed[16, 8])
    exercise_assignments("fast", dt.sc_fixed_fast[16, 8])
    exercise_compounds("fixed", dt.sc_fixed[16, 8])
    exercise_compounds("fast", dt.sc_fixed_fast[16, 8])

    scan_fixed = fixed.range(11, 4)
    scan_fixed.scan(cppyy.gbl.std.istringstream("11010010"))
    emit("scan", "fixed", str(scan_fixed.to_string(dt.SC_BIN, True)), str(fixed.to_bin()))
    scan_fast = fast.range(43, 36)
    scan_fast.scan(cppyy.gbl.std.istringstream("00101101"))
    emit("scan", "fast", str(scan_fast.to_string(dt.SC_BIN, True)), str(fast.to_bin()))

    fixed_view = PROBE.fixed_const_range(fixed, 103, 96)
    fast_view = PROBE.fast_const_range(fast, 48, 41)
    emit(
        "compare",
        "fixed_bv",
        int(fixed_view == dt.sc_bv_base("11111111")),
        int(fixed_view != dt.sc_bv_base("00000000")),
    )
    emit(
        "compare",
        "fast_bv",
        int(fast_view == PROBE.to_fast_bit_vector(fast_view)),
        int(fast_view != dt.sc_bv_base("00000000")),
    )
    emit("concat", "fixed", str(PROBE.concat_fixed(fixed_view, True)))
    emit("concat", "fast", str(PROBE.concat_fast(fast_view, False)))
    exercise_vector_copy_conversions()
    emit("traits", 1, 1, 0, 0)
    deprecated_methods = all(hasattr(fixed_view, name) for name in ("to_signed", "to_unsigned"))
    emit("deprecated_api", "available" if deprecated_methods else "unavailable")
    emit("deprecated_macro", int(PROBE.deprecated_conversions_enabled()))

    exercise_parent_lifelines(dt.sc_fixed[32, 16], "sc_dt::sc_fxnum_subref")
    exercise_parent_lifelines(dt.sc_fixed_fast[32, 16], "sc_dt::sc_fxnum_fast_subref")
