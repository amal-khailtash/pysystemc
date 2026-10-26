"""Compare Python fixed-point bit proxies with a native SystemC reference."""

from pathlib import Path
import gc
import sys
import weakref

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_dt_fixed_bitrefs_probe.h"))
PROBE = cppyy.gbl.sc_dt_fixed_bitrefs_probe


def flatten(value):
    """Replace native dump line endings so a dump stays on one transcript row.

    Args:
        value: Native text copied from a fixed bit proxy's dump method.

    Returns:
        The same text with CR and LF replaced by vertical bars.
    """
    return str(value).replace("\r", "|").replace("\n", "|")


def exercise_mutable_accessors(name, number, index, is_fast):
    """Verify `.bit()` and `[]` writes stay attached to the same parent bit.

    Args:
        name: Transcript label for the fixed-point specialization.
        number: Live owner retained while both borrowed proxy objects are used.
        index: Valid public bit index.
        is_fast: Select the expected fast proxy class when true.
    """
    proxy_type = "sc_dt::sc_fxnum_fast_bitref" if is_fast else "sc_dt::sc_fxnum_bitref"
    method_proxy = number.bit(index)
    item_proxy = number[index]
    assert type(method_proxy).__cpp_name__ == proxy_type
    assert type(item_proxy).__cpp_name__ == proxy_type
    assert method_proxy._pysystemc_parent is number
    initial = bool(method_proxy)
    item_proxy.__assign__(not initial)
    item_write_is_visible = bool(method_proxy)
    method_proxy.__assign__(initial)
    method_write_is_visible = bool(number[index])
    emit(
        "access",
        name,
        index,
        int(initial),
        int(item_write_is_visible),
        int(method_write_is_visible),
        str(number.to_bin()),
    )


def emit(*fields):
    """Emit one row in the shared native/Python transcript format.

    Args:
        *fields: Values appended as tab-separated fields.
    """
    print("FXBIT\t" + "\t".join(str(field) for field in fields))


def exercise_views(name, number, indexes, is_fast):
    """Compare const and mutable views, copies, printing, dumping and scanning.

    Args:
        name: Transcript label for the number family.
        number: Live fixed-point owner borrowed by every proxy in this helper.
        indexes: Valid public indices spanning fractional and integral bits.
        is_fast: Select the const fast-proxy adapters when true.
    """
    readonly = PROBE.fast_const_view_is_readonly(number) if is_fast else PROBE.fixed_const_view_is_readonly(number)
    assert readonly
    for index in indexes:
        const_value = PROBE.read_fast(number, index) if is_fast else PROBE.read_fixed(number, index)
        printed = PROBE.print_fast(number, index) if is_fast else PROBE.print_fixed(number, index)
        dumped = PROBE.dump_fast(number, index) if is_fast else PROBE.dump_fixed(number, index)
        mutable_proxy = number[index]
        assert type(mutable_proxy).__cpp_name__ == (
            "sc_dt::sc_fxnum_fast_bitref" if is_fast else "sc_dt::sc_fxnum_bitref"
        )
        method_proxy = number.bit(index)
        assert type(method_proxy).__cpp_name__ == (
            "sc_dt::sc_fxnum_fast_bitref" if is_fast else "sc_dt::sc_fxnum_bitref"
        )
        emit("view", name, index, int(const_value), str(printed), flatten(dumped), int(bool(mutable_proxy)))

    source_proxy = number[indexes[1]]
    saved_proxy = type(source_proxy)(source_proxy)
    assert type(saved_proxy).__cpp_name__ == ("sc_dt::sc_fxnum_fast_bitref" if is_fast else "sc_dt::sc_fxnum_bitref")
    number[indexes[1]].__assign__(True)
    emit("copy", name, indexes[1], int(bool(saved_proxy)), str(number.to_bin()))

    scan_proxy = number[indexes[1]]
    input_stream = cppyy.gbl.std.istringstream("0")
    scan_proxy.scan(input_stream)
    emit("scan", name, indexes[1], int(bool(scan_proxy)), str(number.to_bin()))
    exercise_mutable_accessors(name, number, indexes[2], is_fast)


def exercise_parent_lifeline(number_type):
    """Keep a borrowed native proxy's parent alive until proxy use ends.

    Args:
        number_type: A fixed-point specialization, including arbitrary width.
    """
    parent = number_type(0)
    reference = weakref.ref(parent)
    proxy = parent.bit(1)
    assert proxy._pysystemc_parent is parent
    del parent
    gc.collect()
    assert reference() is not None
    proxy.__assign__(True)
    owner = proxy._pysystemc_parent
    assert bool(owner[1])
    assert owner.to_double() == 2.0 ** (1 + owner.iwl() - owner.wl())


def exercise_operations(target_name, target_type):
    """Exercise all assignment and bitwise overload source families.

    Args:
        target_name: Transcript label for the mutable destination family.
        target_type: A parameterized fixed-point type used for the destination.
    """
    fixed_source_type = dt.sc_fixed[8, 4]
    fast_source_type = dt.sc_fixed_fast[8, 4]
    source_names = ("fixed", "fast", "sc_bit", "bool")
    source_values = (True, False, True, False)
    operation_names = ("assign", "and", "or", "xor")
    operation_methods = ("__assign__", "__iand__", "__ior__", "__ixor__")

    for source_index, (source_name, right) in enumerate(zip(source_names, source_values, strict=True)):
        for operation_index, (operation_name, method_name) in enumerate(
            zip(operation_names, operation_methods, strict=True)
        ):
            target = target_type(0)
            fixed_source = fixed_source_type(0)
            fast_source = fast_source_type(0)
            bit_source = dt.sc_bit(False)
            initial = operation_index in (1, 3)
            target[1].__assign__(initial)
            fixed_source[1].__assign__(right)
            fast_source[1].__assign__(right)
            bit_source.__assign__(right)

            target_proxy = target[1]
            if source_index == 0:
                source_proxy = fixed_source[1]
            elif source_index == 1:
                source_proxy = fast_source[1]
            elif source_index == 2:
                source_proxy = bit_source
            else:
                source_proxy = right
            getattr(target_proxy, method_name)(source_proxy)
            emit(
                "op",
                target_name,
                operation_name,
                source_name,
                int(initial),
                int(right),
                int(bool(target[1])),
                str(target.to_bin()),
            )


def run_error_mode(mode):
    """Run one isolated native report or fast-width capability probe.

    Args:
        mode: One of the bounded modes used by the focused pytest module.
    """
    previous = sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_THROW)
    try:
        if mode == "bad-index-fixed":
            dt.sc_fixed[16, 8](1).bit(-1)
        elif mode == "bad-index-fast":
            dt.sc_fixed_fast[16, 8](1).bit(16)
        elif mode == "fast-wide":
            number = dt.sc_fixed_fast[137, 100]("9007199254740992.25")
            emit("wide-fast", str(number.to_bin()))
        else:
            raise ValueError(f"unknown fixed bitref mode: {mode}")
    except sc.sc_report as report:
        emit("report", mode, report.get_msg_type(), report.get_msg())
    finally:
        sc.sc_report_handler.set_actions(sc.SC_ERROR, previous)


if __name__ == "__main__" and len(sys.argv) > 1:
    run_error_mode(sys.argv[1])
else:
    wide = dt.sc_fixed[137, 69](-13.375)
    fast = dt.sc_fixed_fast[64, 32](-13.375)
    emit("width", "fixed", wide.wl(), wide.iwl(), str(wide.to_bin()), wide.to_double())
    emit("width", "fast", fast.wl(), fast.iwl(), str(fast.to_bin()), fast.to_double())
    precision_fixed = dt.sc_fixed[137, 100]("9007199254740992.25")
    precision_fast = dt.sc_fixed_fast[137, 100]("9007199254740992.25")
    emit("precision", str(precision_fixed.to_bin()), str(precision_fast.to_bin()))
    exercise_mutable_accessors("fast137", precision_fast, 90, True)

    exercise_views("fixed", wide, (0, 7, 8, 68, 136), False)
    exercise_views("fast", fast, (0, 30, 31, 32, 63), True)

    exercise_operations("fixed", dt.sc_fixed[8, 4])
    exercise_operations("fast", dt.sc_fixed_fast[8, 4])

    exercise_parent_lifeline(dt.sc_fixed[137, 69])
    exercise_parent_lifeline(dt.sc_fixed_fast[64, 32])

    bits = dt.sc_bv_base(2)
    bits[0].__assign__(True)
    bits[1].__assign__(False)
    for index in range(2):
        conversions = PROBE.bitref_conversions(bits, index)
        emit("conv", index, int(conversions.first), int(conversions.second), int(bool(bits[index])))
    emit(
        "proxy_access",
        "fixed_mutable_is_constructible",
        0,
        "fixed_readonly_is_constructible",
        0,
        "fast_mutable_is_constructible",
        0,
        "fast_readonly_is_constructible",
        0,
        "fixed_mutable_copyable",
        1,
        "fixed_readonly_copyable",
        1,
        "fast_mutable_copyable",
        1,
        "fast_readonly_copyable",
        1,
        "fixed_mutable_destructible",
        1,
        "fixed_readonly_destructible",
        1,
        "fast_mutable_destructible",
        1,
        "fast_readonly_destructible",
        1,
    )
