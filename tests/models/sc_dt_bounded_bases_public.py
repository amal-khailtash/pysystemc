"""Exercise public signed and unsigned bounded integer base APIs."""

import gc
from pathlib import Path
import sys

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_bounded_bases_probe.h"))
probe = cppyy.gbl.sc_dt_bounded_bases_probe


def decimal(value):
    """Return the complete signed or unsigned base value as decimal text."""
    return str(value.to_string(dt.SC_DEC))


def emit_value(family, group, label, value):
    """Emit one value plus its native bounded width."""
    print("BAS\t" + "\t".join(map(str, (family, group, label, decimal(value), value.length()))), flush=True)


def emit_fields(family, group, label, *fields):
    """Emit one API observation with its fields in native reference order."""
    print("BAS\t" + "\t".join(map(str, (family, group, label, *fields))), flush=True)


def assign_case(family, label, target, source):
    """Exercise an assignment overload and check reference identity."""
    result = target.__assign__(source)
    emit_fields(family, "assign", label, decimal(target), int(cppyy.addressof(result) == cppyy.addressof(target)))


def constructor_case(family, label, target_type, source):
    """Exercise one public native value constructor."""
    result = target_type(source)
    emit_value(family, "construct", label, result)
    return result


def exercise_compounds(family, base_type, scalar_type):
    """Exercise ten scalar compound methods and four increment methods."""
    value = base_type(scalar_type(48), 32)
    operations = (
        ("plus_assign", "__iadd__", 5),
        ("minus_assign", "__isub__", 3),
        ("multiply_assign", "__imul__", 2),
        ("divide_assign", "__itruediv__", 4),
        ("modulo_assign", "__imod__", 7),
        ("and_assign", "__iand__", 6),
        ("or_assign", "__ior__", 8),
        ("xor_assign", "__ixor__", 3),
        ("left_shift_assign", "__ilshift__", 2),
        ("right_shift_assign", "__irshift__", 1),
    )
    for label, method_name, scalar in operations:
        result = getattr(value, method_name)(scalar_type(scalar))
        emit_fields(
            family,
            "compound",
            label,
            decimal(value),
            int(cppyy.addressof(result) == cppyy.addressof(value)),
        )

    prefix_up = value.__preinc__()
    emit_fields(
        family,
        "compound",
        "prefix_increment",
        decimal(value),
        int(cppyy.addressof(prefix_up) == cppyy.addressof(value)),
    )
    postfix_up = value.__postinc__(0)
    emit_fields(family, "compound", "postfix_increment", decimal(postfix_up), decimal(value))
    prefix_down = value.__predec__()
    emit_fields(
        family,
        "compound",
        "prefix_decrement",
        decimal(value),
        int(cppyy.addressof(prefix_down) == cppyy.addressof(value)),
    )
    postfix_down = value.__postdec__(0)
    emit_fields(family, "compound", "postfix_decrement", decimal(postfix_down), decimal(value))


def make_sources():
    """Build independently owned sources and live range/concat parents."""
    signed = dt.sc_signed(48)
    signed.__assign__("-123456789")
    unsigned = dt.sc_unsigned(48)
    unsigned.__assign__("234567890")
    signed_range = probe.const_signed_range(signed, 31, 4)
    unsigned_range = probe.const_unsigned_range(unsigned, 31, 4)
    signed_fixed = dt.sc_int[24](-345678)
    unsigned_fixed = dt.sc_uint[24](456789)
    int_range = probe.const_int_range(signed_fixed, 19, 3)
    uint_range = probe.const_uint_range(unsigned_fixed, 19, 3)
    bit_vector = dt.sc_bv[8]("10100101")
    logic_vector = dt.sc_lv[8]("01101010")
    generic_signed = dt.sc_signed(8)
    generic_signed.__assign__(0xAB)
    generic_unsigned = dt.sc_unsigned(8)
    generic_unsigned.__assign__(0x35)
    generic = dt.concat(generic_signed, generic_unsigned)
    owners = [
        signed,
        unsigned,
        signed_fixed,
        unsigned_fixed,
        bit_vector,
        logic_vector,
        generic_signed,
        generic_unsigned,
    ]
    sources = {
        "signed": signed,
        "unsigned": unsigned,
        "signed_subref": signed_range,
        "unsigned_subref": unsigned_range,
        "int_subref": int_range,
        "uint_subref": uint_range,
        "bv": bit_vector,
        "lv": logic_vector,
        "generic": generic,
    }
    return sources, owners


def exercise_base(family, base_type, scalar_type, signed):
    """Exercise constructors, assignments, methods and exact-width behavior."""
    scalar = probe.signed_scalar() if signed else probe.unsigned_scalar()
    native_scalar = probe.SignedScalar(scalar) if signed else probe.UnsignedScalar(scalar)
    expected_cpp_type = (
        "sc_dt_bounded_bases_probe::SignedScalar" if signed else "sc_dt_bounded_bases_probe::UnsignedScalar"
    )
    assert type(native_scalar).__cpp_name__ == expected_cpp_type
    assert type(scalar_type(1)).__cpp_name__ == expected_cpp_type

    default = base_type()
    emit_value(family, "construct", "default", default)
    width_default = base_type(32)
    emit_value(family, "construct", "width", width_default)
    scalar_value = base_type(native_scalar, 32)
    emit_value(family, "construct", "scalar_width", scalar_value)
    emit_value(family, "construct", "copy", base_type(scalar_value))

    sources, owners = make_sources()
    constructed = []
    for label in (
        "signed",
        "unsigned",
        "signed_subref",
        "unsigned_subref",
        "int_subref",
        "uint_subref",
        "bv",
        "lv",
    ):
        constructed.append(constructor_case(family, label, base_type, sources[label]))
    generic_constructed = constructor_case(family, "generic", base_type, sources["generic"])
    constructed.append(generic_constructed)

    target = base_type(32)
    assign_case(family, "base", target, scalar_value)
    assignment_labels = [
        "signed",
        "unsigned",
        "int_subref" if signed else "uint_subref",
        "bv",
        "lv",
        "string",
        "unsigned_long",
        "long",
        "unsigned_int",
        "int",
        "double",
        "generic",
    ]
    for label in assignment_labels:
        if label in sources:
            source = sources[label]
        elif label == "string":
            source = "-765432109876"
        elif label == "unsigned_long":
            source = probe.UnsignedLong(345678901)
        elif label == "long":
            source = probe.SignedLong(-123456789)
        elif label == "unsigned_int":
            source = probe.UnsignedInt(2345678)
        elif label == "int":
            source = probe.SignedInt(-1234567)
        else:
            source = probe.Double(-9876.75)
        assign_case(family, label, target, source)

    if signed:
        assign_case(family, "scalar_int64", target, probe.SignedScalar(scalar))
        assign_case(family, "scalar_uint64", target, probe.UnsignedScalar(probe.unsigned_scalar()))
    else:
        assign_case(family, "scalar_uint64", target, probe.UnsignedScalar(scalar))
        assign_case(family, "scalar_int64", target, probe.SignedScalar(probe.signed_scalar()))

    if probe.fixed_point_enabled():
        fixed_sources = (
            ("fxval", dt.sc_fxval(23.75)),
            ("fxval_fast", dt.sc_fxval_fast(31.5)),
            ("fxnum", dt.sc_fixed[20, 12](41.75)),
            ("fxnum_fast", dt.sc_fixed_fast[20, 12](57.25)),
        )
        for label, source in fixed_sources:
            assign_case(family, label, target, source)

    exercise_compounds(family, base_type, scalar_type)

    target.__assign__(scalar_type(0x12345))
    emit_fields(
        family,
        "observe",
        "numeric",
        target.value(),
        target.to_int(),
        target.to_uint(),
        target.to_long(),
        target.to_ulong(),
        target.to_int64(),
        target.to_uint64(),
        f"{target.to_double():.6f}",
        target.long_low(),
        target.long_high(),
    )
    emit_fields(
        family,
        "observe",
        "reductions",
        int(target.and_reduce()),
        int(target.nand_reduce()),
        int(target.or_reduce()),
        int(target.nor_reduce()),
        int(target.xor_reduce()),
        int(target.xnor_reduce()),
    )
    emit_fields(family, "observe", "strings", target.to_string(dt.SC_BIN), target.to_string(dt.SC_HEX, True))
    printed = cppyy.gbl.std.ostringstream()
    target.print(printed)
    emit_fields(family, "observe", "print", printed.str())

    target.__assign__(scalar_type(0))
    target.set(3)
    emit_value(family, "bits", "set_one_arg", target)
    target.set(3, False)
    emit_value(family, "bits", "set_bool", target)
    target.set(4, True)
    emit_fields(family, "bits", "test", int(target.test(4)), int(target.test(3)))

    target.__assign__(scalar_type(0x5A35))
    digits = cppyy.gbl.std.array[cppyy.gbl.sc_dt.sc_digit, 2]()
    controls = cppyy.gbl.std.array[cppyy.gbl.sc_dt.sc_digit, 2]()
    digits[0] = digits[1] = 0
    controls[0] = controls[1] = 0
    xz_present = cppyy.gbl.std.array[bool, 1]()
    xz_present[0] = True
    packed_length = target.concat_length(xz_present.data())
    ctrl_present = target.concat_get_ctrl(controls.data(), 0)
    data_present = target.concat_get_data(digits.data(), 0)
    emit_fields(
        family,
        "concat",
        "pack",
        packed_length,
        int(xz_present[0]),
        int(ctrl_present),
        controls[0],
        int(data_present),
        digits[0],
        target.concat_get_uint64(),
    )
    target.concat_set(probe.SignedScalar(5), 3)
    emit_value(family, "concat", "set_int64", target)
    target.__assign__(scalar_type(0))
    target.concat_set(probe.UnsignedScalar(6), 3)
    emit_value(family, "concat", "set_uint64", target)
    target.__assign__(scalar_type(0))
    signed_source = sources["signed"]
    target.concat_set(signed_source, 4)
    emit_value(family, "concat", "set_signed", target)
    target.__assign__(scalar_type(0))
    target.concat_set(sources["unsigned"], 4)
    emit_value(family, "concat", "set_unsigned", target)

    target.__assign__(scalar_type(0x12345))
    index = 7
    mutable_subscript = target[index]
    mutable_bit = target.bit(index)
    const_bit = probe.const_signed_bit if signed else probe.const_unsigned_bit
    const_bit_method = probe.const_signed_bit_method if signed else probe.const_unsigned_bit_method
    read_subscript = const_bit(target, index)
    read_bit = const_bit_method(target, index)
    emit_fields(
        family,
        "access",
        "bit",
        mutable_subscript.length(),
        int(mutable_bit.value()),
        read_subscript.length(),
        int(read_subscript.value()),
        int(read_bit.value()),
    )
    mutable_call_length = target(index, 2).length()
    mutable_range_length = target.range(index, 2).length()
    const_range = probe.const_signed_range if signed else probe.const_unsigned_range
    const_call = probe.const_signed_call if signed else probe.const_unsigned_call
    read_call_length = const_call(target, index, 2).length()
    read_range_length = const_range(target, index, 2).length()
    emit_fields(
        family,
        "access",
        "range",
        mutable_call_length,
        decimal(target),
        mutable_range_length,
        read_call_length,
        read_range_length,
    )

    target.__assign__(scalar_type(0x5A35))
    input_stream = cppyy.gbl.std.istringstream("-27")
    target.scan(input_stream)
    emit_fields(family, "stream", "scan", decimal(target), int(input_stream.rdstate()))
    old_actions = probe.throw_errors()
    invalid_stream = cppyy.gbl.std.istringstream("invalid")
    caught = False
    try:
        target.scan(invalid_stream)
    except cppyy.gbl.sc_core.sc_report:
        caught = True
    finally:
        probe.restore_errors(old_actions)
    emit_fields(family, "stream", "scan_invalid", decimal(target), int(invalid_stream.rdstate()), int(caught))

    for width in (1, 19, 59, 63, 64):
        value = base_type(scalar_type(-0x12345 if signed else 0xFEDC), width)
        emit_value(family, "width", width, value)
        copied = base_type(value)
        value.__assign__(scalar_type(0))
        emit_value(family, "width_copy", width, copied)

    old_actions = probe.throw_errors()
    try:
        for width in (0, -1, 65):
            caught = False
            report_type = ""
            report_message = ""
            message = "sc_int[_base]" if signed else "sc_uint[_base]"
            expected_detail = f"{message} initialization: length = {width} violates 1 <= length <= 64"
            expected_type = str(probe.out_of_bounds_msg_type())
            try:
                base_type(width)
            except cppyy.gbl.sc_core.sc_report as report:
                caught = True
                report_type = str(report.get_msg_type())
                report_message = str(report.get_msg())
            except TypeError as error:
                diagnostic = str(error)
                if expected_type not in diagnostic or expected_detail not in diagnostic:
                    raise
                caught = True
                report_type = expected_type
                report_message = expected_detail
            assert caught and report_type == expected_type and report_message == expected_detail
            emit_fields(family, "invalid_width", width, int(caught), report_type, report_message)
    finally:
        probe.restore_errors(old_actions)

    lifetime_source = dt.sc_signed(48)
    lifetime_source.__assign__("-123456789")
    lifetime_copy = base_type(lifetime_source)
    lifetime_text = decimal(lifetime_copy)
    del lifetime_source
    gc.collect()
    assert decimal(lifetime_copy) == lifetime_text
    emit_value(family, "lifetime", "source_released", lifetime_copy)
    del constructed, owners, sources, generic_constructed, scalar_value, target, width_default, default
    gc.collect()


assert type(probe.SignedScalar(probe.signed_scalar())).__cpp_name__ == "sc_dt_bounded_bases_probe::SignedScalar"
assert type(probe.UnsignedScalar(probe.unsigned_scalar())).__cpp_name__ == "sc_dt_bounded_bases_probe::UnsignedScalar"
for scalar_alias, expected_name in (
    (probe.SignedInt, "sc_dt_bounded_bases_probe::SignedInt"),
    (probe.UnsignedInt, "sc_dt_bounded_bases_probe::UnsignedInt"),
    (probe.SignedLong, "sc_dt_bounded_bases_probe::SignedLong"),
    (probe.UnsignedLong, "sc_dt_bounded_bases_probe::UnsignedLong"),
    (probe.Double, "sc_dt_bounded_bases_probe::Double"),
):
    assert type(scalar_alias(1)).__cpp_name__ == expected_name
FAMILIES = {
    "sc_dt::sc_int_base": (dt.sc_int_base, probe.SignedScalar, True),
    "sc_dt::sc_uint_base": (dt.sc_uint_base, probe.UnsignedScalar, False),
}
selected_families = sys.argv[1:] or list(FAMILIES)
if not selected_families or any(family not in FAMILIES for family in selected_families):
    raise ValueError(f"expected one or more names from {tuple(FAMILIES)}")
for selected_family in selected_families:
    exercise_base(selected_family, *FAMILIES[selected_family])
print("SC_DT_BOUNDED_BASES_PUBLIC_OK")
