"""Exercise borrowed sc_dt vector subreferences and concatenations."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_dt_vector_subrefs_probe.h"))

PROBE = cppyy.gbl.sc_dt_vector_subrefs_probe


def pattern(width, alphabet, offset):
    """Build the deterministic native-comparable spelling for a wide vector."""
    return "".join(alphabet[(index * 7 + offset) % len(alphabet)] for index in range(width))


def emit(family, case_name, *values):
    """Print one exact record consumed by the independent native comparator."""
    print("\t".join(("SDR", family, case_name, *(str(value) for value in values))))


def render(view):
    """Return the native SystemC spelling of a vector or borrowed proxy."""
    return str(view)


def emit_range(kind, direction, view, trait_helper):
    """Emit public range shape, protected-state, trait, and print observations."""
    emit(
        "range",
        kind,
        direction,
        int(view.length()),
        int(view.size()),
        int(bool(view.reversed())),
        int(bool(view.is_01())),
        render(view),
    )
    emit("state", kind, direction, PROBE.range_state(view))
    emit("traits", kind, direction, trait_helper())
    stream = cppyy.gbl.std.ostringstream()
    view.print(stream)
    emit("print", kind, direction, str(stream.str()))


def emit_mutable_ranges(bv, lv):
    """Test public proxy copy construction, assignment, and bitwise compounds."""
    bv_view = bv.range(47, 32)
    bv_copy = type(bv_view)(bv_view)
    bv_copy.__assign__("1010010110100101")
    emit("assign", "bv_copy", render(bv_view), render(bv))
    bv_copy.__iand__("1111000011110000")
    emit("compound", "bv_and", render(bv_view), render(bv))
    bv_copy.__ior__("0000111100001111")
    emit("compound", "bv_or", render(bv_view), render(bv))
    bv_copy.__ixor__("0011001100110011")
    emit("compound", "bv_xor", render(bv_view), render(bv))

    lv_view = lv.range(55, 40)
    lv_copy = type(lv_view)(lv_view)
    lv_copy.__assign__("ZX01ZX01ZX01ZX01")
    emit("assign", "lv_copy", render(lv_view), render(lv))
    lv_copy.__iand__("11ZZ00XX11ZZ00XX")
    emit("compound", "lv_and", render(lv_view), render(lv))
    lv_copy.__ior__("0011XXZZ0011XXZZ")
    emit("compound", "lv_or", render(lv_view), render(lv))
    lv_copy.__ixor__("01ZX01ZX01ZX01ZX")
    emit("compound", "lv_xor", render(lv_view), render(lv))


def emit_concatenations(bv, lv, lv_second):
    """Test writable, read-only, copied, and nested borrowed concatenations."""
    left = bv.range(31, 16)
    middle = lv.range(39, 24)
    right = lv_second.range(47, 32)
    concatenated = dt.concat(left, middle)
    emit("concat", "mutable", int(concatenated.length()), render(concatenated))
    emit("state", "concat_mutable", PROBE.concat_state(concatenated))
    emit("traits", "concat_mutable", PROBE.concat_mutable_traits())
    stream = cppyy.gbl.std.ostringstream()
    concatenated.print(stream)
    emit("print", "concat_mutable", str(stream.str()))
    mutable_direct = type(concatenated)(left, middle)
    emit("construct", "concat_mutable_direct", render(mutable_direct))
    concatenated.__assign__("101010101010101001ZX01ZX01ZX01ZX")
    emit("concat", "assigned", render(left), render(middle), render(concatenated))
    mutable_copy = type(concatenated)(concatenated)
    mutable_copy.__assign__("00110011001100110011001100110011")
    emit("concat", "copy_assigned", render(left), render(middle), render(mutable_copy))

    nested_inner = dt.concat(left, middle)
    nested_outer = dt.concat(nested_inner, right)
    emit("concat", "nested", int(nested_outer.length()), render(nested_outer))

    readonly_left = dt.sc_subref_r[dt.sc_bv_base](bv, 31, 16)
    readonly_right = dt.sc_subref_r[dt.sc_lv_base](lv, 39, 24)
    readonly_concat = dt.concat(readonly_left, readonly_right)
    emit(
        "concat", "readonly", int(readonly_concat.length()), render(readonly_concat), int(bool(readonly_concat.is_01()))
    )
    emit("state", "concat_readonly", PROBE.concat_state(readonly_concat))
    emit("traits", "concat_readonly", PROBE.concat_readonly_traits())
    stream = cppyy.gbl.std.ostringstream()
    readonly_concat.print(stream)
    emit("print", "concat_readonly", str(stream.str()))
    readonly_direct = type(readonly_concat)(readonly_left, readonly_right)
    emit("construct", "concat_readonly_direct", render(readonly_direct))

    readonly_copy = type(readonly_concat)(readonly_concat)
    emit("construct", "concat_readonly_copy", render(readonly_copy))
    emit("construct", "concat_mutable_copy", render(type(concatenated)(concatenated)))


def apply_exact_assignment(destination, signature, value):
    """Select one declared operator= signature and assign its exact input type."""
    destination.__assign__.__overload__(signature)(destination, value)


def emit_subref_assignment_matrix(kind, base_type, destination_owner, source_owner):
    """Invoke every inventoried sc_subref operator= overload on one vector type."""
    destination = destination_owner.range(15, 0)
    source_write = source_owner.range(15, 0)
    source_read = dt.sc_subref_r[base_type](source_owner, 15, 0)
    proxy_signature = "const sc_dt::sc_proxy<sc_dt::sc_bv_base>&"
    if base_type is dt.sc_lv_base:
        proxy_signature = "const sc_dt::sc_proxy<sc_dt::sc_bv_base>&"
    cases = (
        ("proxy_template", proxy_signature, PROBE.assignment_proxy_value()),
        ("subref_readonly_copy", f"const sc_dt::sc_subref_r<{base_type.__cpp_name__}>&", source_read),
        ("subref_mutable_copy", f"const sc_dt::sc_subref<{base_type.__cpp_name__}>&", source_write),
        ("bool_pointer", "const bool*", PROBE.assignment_bool_bits()),
        ("char_pointer", "const char*", "1010010110100101"),
        ("logic_pointer", "const sc_dt::sc_logic*", PROBE.assignment_logic_bits()),
        ("sc_unsigned", "const sc_dt::sc_unsigned&", PROBE.assignment_unsigned_value()),
        ("sc_signed", "const sc_dt::sc_signed&", PROBE.assignment_signed_value()),
        ("sc_uint_base", "const sc_dt::sc_uint_base&", PROBE.assignment_uint_value()),
        ("sc_int_base", "const sc_dt::sc_int_base&", PROBE.assignment_int_value()),
        ("unsigned_long", "unsigned long", 0x12345678),
        ("long", "long", -0x13579),
        ("unsigned_int", "unsigned int", 0xA53C),
        ("int", "int", 0x5A35),
        ("uint64", "sc_dt::uint64", 0x123456789ABCDEF0),
        ("int64", "sc_dt::int64", -0x123456789),
    )
    for label, signature, value in cases:
        apply_exact_assignment(destination, "const char*", "0" * 16)
        apply_exact_assignment(destination, signature, value)
        emit("assignment", kind, label, render(destination))


def emit_concat_assignment_matrix():
    """Invoke each declared sc_concref operator= overload with typed inputs."""
    destination_bv = dt.sc_bv[32]("0" * 32)
    destination_lv = dt.sc_lv[32]("0" * 32)
    source_bv = dt.sc_bv[32]("10100101101001011100110000111100")
    source_lv = dt.sc_lv[32]("10100101101001011100110000111100")
    destination = dt.concat(destination_bv.range(31, 16), destination_lv.range(31, 16))
    source_copy = dt.concat(source_bv.range(31, 16), source_lv.range(31, 16))
    copy_signature = f"const {type(destination).__cpp_name__}&"
    cases = (
        ("proxy_template", "const sc_dt::sc_proxy<sc_dt::sc_bv_base>&", PROBE.assignment_proxy_value()),
        ("concref_copy", copy_signature, source_copy),
        ("bool_pointer", "const bool*", PROBE.assignment_bool_bits()),
        ("char_pointer", "const char*", "10100101101001011100110000111100"),
        ("logic_pointer", "const sc_dt::sc_logic*", PROBE.assignment_logic_bits()),
        ("sc_unsigned", "const sc_dt::sc_unsigned&", PROBE.assignment_unsigned_value()),
        ("sc_signed", "const sc_dt::sc_signed&", PROBE.assignment_signed_value()),
        ("sc_uint_base", "const sc_dt::sc_uint_base&", PROBE.assignment_uint_value()),
        ("sc_int_base", "const sc_dt::sc_int_base&", PROBE.assignment_int_value()),
        ("unsigned_long", "unsigned long", 0x12345678),
        ("long", "long", -0x13579),
        ("unsigned_int", "unsigned int", 0xA53C),
        ("int", "int", 0x5A35),
        ("uint64", "sc_dt::uint64", 0x123456789ABCDEF0),
        ("int64", "sc_dt::int64", -0x123456789),
    )
    for label, signature, value in cases:
        apply_exact_assignment(destination, "const char*", "0" * 32)
        apply_exact_assignment(destination, signature, value)
        emit("assignment", "concat", label, render(destination))


def emit_proxy_storage(family, kind, view):
    """Call public bit/word/cword accessors, mutators, and tail cleanup."""
    emit("storage", family, kind, "get_bit", int(view.get_bit(0)))
    view.set_bit(0, dt.SC_LOGIC_1.value() if kind == "lv" else True)
    emit("storage", family, kind, "set_bit", render(view))
    view.set_word(0, 0xA5)
    emit("storage", family, kind, "set_word", int(view.get_word(0)), int(view.get_cword(0)), render(view))
    view.set_cword(0, 0x5A)
    emit("storage", family, kind, "set_cword", int(view.get_word(0)), int(view.get_cword(0)), render(view))
    view.clean_tail()
    emit("storage", family, kind, "clean_tail", render(view))


def emit_clone(family, kind, view):
    """Take Python ownership of a native clone and release it while its parent lives."""
    clone = view.clone()
    clone.__python_owns__ = True
    emit("clone", family, kind, render(clone))
    del clone
    gc.collect()


def emit_low_level_surfaces():
    """Exercise all declared accessors, setters, clone methods, and destructors."""
    bv = dt.sc_bv[64]("0101010101010101010101010101010101010101010101010101010101010101")
    lv = dt.sc_lv[64]("01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX01ZX")
    bv_write = bv.range(31, 16)
    lv_write = lv.range(31, 16)
    bv_read = PROBE.readonly_bv_range(bv, 31, 16)
    lv_read = PROBE.readonly_lv_range(lv, 31, 16)
    for family, kind, view in (
        ("sc_subref", "bv", bv_write),
        ("sc_subref", "lv", lv_write),
        ("sc_subref_r", "bv", bv_read),
        ("sc_subref_r", "lv", lv_read),
    ):
        emit_proxy_storage(family, kind, view)
    for family, kind, view in (
        ("sc_subref", "bv", bv_write),
        ("sc_subref", "lv", lv_write),
        ("sc_subref_r", "bv", bv_read),
        ("sc_subref_r", "lv", lv_read),
    ):
        emit_clone(family, kind, view)

    concat_write = dt.concat(bv.range(31, 16), lv.range(31, 16))
    concat_read = dt.concat(bv_read, lv_read)
    emit_proxy_storage("sc_concref", "mixed", concat_write)
    emit_proxy_storage("sc_concref_r", "mixed", concat_read)
    emit_clone("sc_concref", "mixed", concat_write)
    emit_clone("sc_concref_r", "mixed", concat_read)


def run():
    """Run all four proxy families with their borrowed owners kept in scope."""
    bv = dt.sc_bv[137](pattern(137, "01", 3))
    lv = dt.sc_lv[191](pattern(191, "01ZX", 1))
    lv_second = dt.sc_lv[191](pattern(191, "01ZX", 2))

    bv_source_range = bv.range(31, 16)
    bv_direct = type(bv_source_range)(bv, 31, 16)
    bv_copy = type(bv_source_range)(bv_source_range)
    bv_readonly_direct = dt.sc_subref_r[dt.sc_bv_base](bv, 31, 16)
    emit("construct", "bv_mutable_direct", render(bv_direct))
    emit("construct", "bv_mutable_copy", render(bv_copy))
    emit("construct", "bv_readonly_direct", render(bv_readonly_direct))
    lv_source_range = lv.range(31, 16)
    lv_direct = type(lv_source_range)(lv, 31, 16)
    lv_copy = type(lv_source_range)(lv_source_range)
    lv_readonly_direct = dt.sc_subref_r[dt.sc_lv_base](lv, 31, 16)
    emit("construct", "lv_mutable_direct", render(lv_direct))
    emit("construct", "lv_mutable_copy", render(lv_copy))
    emit("construct", "lv_readonly_direct", render(lv_readonly_direct))

    live_view = bv.range(111, 96)
    before_owner_change = render(live_view)
    del bv_source_range, bv_direct, bv_copy, bv_readonly_direct
    gc.collect()
    bv.range(111, 96).__assign__("0011001100110011")
    emit("borrowed", "parent_mutation", before_owner_change, render(live_view))

    traits = {
        "bv137": PROBE.bv_mutable_traits,
        "lv191": PROBE.lv_mutable_traits,
        "bv137_readonly": PROBE.bv_readonly_traits,
        "lv191_readonly": PROBE.lv_readonly_traits,
    }
    for high, low, direction in ((31, 16, "forward"), (16, 31, "reversed")):
        for kind, owner, constructor, trait_helper in (
            ("bv137", bv, dt.sc_subref[dt.sc_bv_base], traits["bv137"]),
            ("lv191", lv, dt.sc_subref[dt.sc_lv_base], traits["lv191"]),
            ("bv137_readonly", bv, dt.sc_subref_r[dt.sc_bv_base], traits["bv137_readonly"]),
            ("lv191_readonly", lv, dt.sc_subref_r[dt.sc_lv_base], traits["lv191_readonly"]),
        ):
            view = constructor(owner, high, low)
            emit_range(kind, direction, view, trait_helper)

    numeric_bv = bv.range(63, 48)
    emit("convert", "bv", int(numeric_bv.to_uint64()), int(numeric_bv.to_int64()), str(numeric_bv.to_string()))
    numeric_lv = lv.range(63, 48)
    emit("convert", "lv", int(numeric_lv.to_uint64()), int(numeric_lv.to_int64()), str(numeric_lv.to_string()))
    emit("construct", "compile_time", int(bool(PROBE.construction_traits())))

    try:
        bv.range(int(bv.length()), 0)
    except Exception:
        rejected = 1
    else:
        rejected = 0
    emit("error", "invalid_range_rejected", rejected)
    emit("concat", "nested_native_helper", str(PROBE.nested_read_concat(bv, lv, lv_second)))

    emit_mutable_ranges(bv, lv)
    emit_concatenations(bv, lv, lv_second)

    scanned = bv.range(95, 80)
    scan_stream = cppyy.gbl.std.istringstream("1011001110001111")
    scanned.scan(scan_stream)
    emit("scan", "bv", render(scanned), render(bv))
    lv_scanned = lv.range(95, 80)
    lv_scan_stream = cppyy.gbl.std.istringstream("ZX01ZX01ZX01ZX01")
    lv_scanned.scan(lv_scan_stream)
    emit("scan", "lv", render(lv_scanned), render(lv))
    concat_scanned = dt.concat(bv.range(111, 96), lv.range(111, 96))
    concat_scan_stream = cppyy.gbl.std.istringstream("11001100110011000011XXZZ0011XXZZ")
    concat_scanned.scan(concat_scan_stream)
    emit("scan", "concat", render(concat_scanned), render(bv), render(lv))

    bv_assignment_destination = dt.sc_bv[64]("0")
    bv_assignment_source = dt.sc_bv[64](pattern(64, "01", 2))
    lv_assignment_destination = dt.sc_lv[64]("0")
    lv_assignment_source = dt.sc_lv[64](pattern(64, "01ZX", 3))
    emit_subref_assignment_matrix("bv_subref", dt.sc_bv_base, bv_assignment_destination, bv_assignment_source)
    emit_subref_assignment_matrix("lv_subref", dt.sc_lv_base, lv_assignment_destination, lv_assignment_source)
    emit_concat_assignment_matrix()
    emit_low_level_surfaces()


if __name__ == "__main__":
    run()
