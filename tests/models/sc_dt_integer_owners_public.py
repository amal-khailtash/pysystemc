"""Exercise arbitrary-width signed and unsigned SystemC integer owners."""

import gc
from pathlib import Path
import sys

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_integer_owners_probe.h"))
probe = cppyy.gbl.sc_dt_integer_owners_probe


def decimal(value):
    """Return an arbitrary-width integer as exact native decimal text."""
    return str(value.to_string(dt.SC_DEC))


def emit(family, group, label, *fields):
    """Emit one machine-readable observation in the native reference order."""
    print("OWN\t" + "\t".join(map(str, (family, group, label, *fields))), flush=True)


def emit_value(family, group, label, value):
    """Emit decimal value and declared width for one native owner."""
    emit(family, group, label, decimal(value), value.length())


def sources():
    """Create owned values and borrowed views with all parents kept alive."""
    signed = dt.sc_signed(257)
    signed.__assign__("-123456789012345678901234567890123456789")
    unsigned = dt.sc_unsigned(257)
    unsigned.__assign__("987654321098765432109876543210987654321")
    signed_fixed = dt.sc_int_base(64)
    signed_fixed.__assign__(probe.SignedScalar(-0x123456789AB))
    unsigned_fixed = dt.sc_uint_base(64)
    unsigned_fixed.__assign__(probe.UnsignedScalar(0xFEDCBA9876543210))
    bit_vector = dt.sc_bv[64]("1010010110100101101001011010010110100101101001011010010110100101")
    logic_vector = dt.sc_lv[64]("0110101001101010011010100110101001101010011010100110101001101010")
    generic_signed = dt.sc_signed(16)
    generic_signed.__assign__(0xA55A)
    generic_unsigned = dt.sc_unsigned(16)
    generic_unsigned.__assign__(0x35C3)
    generic = dt.concat(generic_signed, generic_unsigned)
    views = {
        "signed_range": probe.const_signed_range(signed, 127, 64),
        "unsigned_range": probe.const_unsigned_range(unsigned, 127, 64),
        "int_range": probe.const_int_range(signed_fixed, 63, 16),
        "uint_range": probe.const_uint_range(unsigned_fixed, 63, 16),
    }
    values = {
        "signed": signed,
        "unsigned": unsigned,
        "sc_int_base": signed_fixed,
        "sc_uint_base": unsigned_fixed,
        "bv": bit_vector,
        "lv": logic_vector,
        "generic_concat": generic,
        **views,
    }
    owners = (signed, unsigned, signed_fixed, unsigned_fixed, bit_vector, logic_vector, generic_signed, generic_unsigned)
    return values, owners


def exercise_constructors():
    """Check owner constructors, assignment overloads and copied storage."""
    families = {
        "sc_dt::sc_signed": dt.sc_signed,
        "sc_dt::sc_unsigned": dt.sc_unsigned,
    }
    for family, owner_type in families.items():
        width = owner_type(32)
        emit_value(family, "construct", "width", width)
        emit_value(family, "construct", "copy", owner_type(width))
        values, owners = sources()
        labels = (
            "signed", "unsigned", "signed_range", "unsigned_range", "int_range", "uint_range", "bv", "lv",
            "generic_concat",
        )
        for label in labels:
            emit_value(family, "construct", label, owner_type(values[label]))

        target = owner_type(257)
        for label in (
            "signed", "unsigned", "signed_range", "unsigned_range", "sc_int_base", "sc_uint_base", "bv", "lv",
            "generic_concat",
        ):
            result = target.__assign__(values[label])
            emit(family, "assign", label, decimal(target), int(cppyy.addressof(result) == cppyy.addressof(target)))
        scalar_sources = (
            ("string", "-765432109876543210987654321"),
            ("int", probe.SignedInt(-1234567)),
            ("unsigned_int", probe.UnsignedInt(2345678)),
            ("long", probe.SignedLong(-123456789)),
            ("unsigned_long", probe.UnsignedLong(345678901)),
            ("int64", probe.SignedScalar(probe.signed_scalar())),
            ("uint64", probe.UnsignedScalar(probe.unsigned_scalar())),
            ("double", probe.Double(-9876.75)),
        )
        for label, source in scalar_sources:
            result = target.__assign__(source)
            emit(family, "assign", label, decimal(target), int(cppyy.addressof(result) == cppyy.addressof(target)))

        if probe.fixed_point_enabled():
            fixed_sources = (
                ("fxval", dt.sc_fxval(23.75)),
                ("fxval_fast", dt.sc_fxval_fast(31.5)),
                ("fxnum", dt.sc_fixed[24, 12](41.75)),
                ("fxnum_fast", dt.sc_fixed_fast[24, 12](57.25)),
            )
            for label, source in fixed_sources:
                target.__assign__(source)
                emit(family, "assign", label, decimal(target), 1)

        retained = owner_type(values["signed"])
        retained_text = decimal(retained)
        del values, owners
        gc.collect()
        emit(family, "lifetime", "copied_owner", retained_text, retained.length())


def operand_types():
    """Return one live, exact native object for each compound signature type."""
    signed_fixed = dt.sc_int_base(32)
    signed_fixed.__assign__(3)
    unsigned_fixed = dt.sc_uint_base(32)
    unsigned_fixed.__assign__(3)
    signed = dt.sc_signed(32)
    signed.__assign__(3)
    unsigned = dt.sc_unsigned(32)
    unsigned.__assign__(3)
    return (
        ("int", probe.SignedInt(3)),
        ("unsigned_int", probe.UnsignedInt(3)),
        ("long", probe.SignedLong(3)),
        ("unsigned_long", probe.UnsignedLong(3)),
        ("int64", probe.SignedScalar(3)),
        ("uint64", probe.UnsignedScalar(3)),
        ("sc_int_base", signed_fixed),
        ("sc_uint_base", unsigned_fixed),
        ("sc_signed", signed),
        ("sc_unsigned", unsigned),
    )


def exercise_compounds():
    """Exercise every exact compound operand overload and both increment forms."""
    operations = (
        ("add", "__iadd__"), ("subtract", "__isub__"), ("multiply", "__imul__"), ("divide", "__itruediv__"),
        ("remainder", "__imod__"), ("and", "__iand__"), ("or", "__ior__"), ("xor", "__ixor__"),
        ("left_shift", "__ilshift__"), ("right_shift", "__irshift__"),
    )
    for family, owner_type in (("sc_dt::sc_signed", dt.sc_signed), ("sc_dt::sc_unsigned", dt.sc_unsigned)):
        for operand_label, operand in operand_types():
            for label, method_name in operations:
                target = owner_type(257)
                target.__assign__(80)
                result = getattr(target, method_name)(operand)
                emit(
                    family,
                    "compound",
                    f"{label}_{operand_label}",
                    decimal(target),
                    int(cppyy.addressof(result) == cppyy.addressof(target)),
                )
        value = owner_type(257)
        value.__assign__(80)
        prefix = value.__preinc__()
        emit(family, "increment", "prefix_up", decimal(value), int(cppyy.addressof(prefix) == cppyy.addressof(value)))
        postfix = value.__postinc__(0)
        emit(family, "increment", "postfix_up", decimal(postfix), decimal(value))
        prefix = value.__predec__()
        emit(family, "increment", "prefix_down", decimal(value), int(cppyy.addressof(prefix) == cppyy.addressof(value)))
        postfix = value.__postdec__(0)
        emit(family, "increment", "postfix_down", decimal(postfix), decimal(value))


def digit_array(count):
    """Allocate zeroed native digit storage for a public pointer API call."""
    values = cppyy.gbl.std.vector[cppyy.gbl.sc_dt.sc_digit](count)
    for index in range(count):
        values[index] = 0
    return values


def exercise_storage():
    """Check observers, bit and range views, packing, concatenation and widths."""
    families = (
        ("sc_dt::sc_signed", dt.sc_signed, True),
        ("sc_dt::sc_unsigned", dt.sc_unsigned, False),
    )
    for family, owner_type, signed in families:
        value = owner_type(257)
        scalar = probe.signed_scalar() if signed else probe.unsigned_scalar()
        value.__assign__(scalar)
        emit(
            family,
            "observe",
            "numeric",
            decimal(value),
            value.length(),
            int(value.sign()),
            int(value.iszero()),
            value.to_int(),
            value.to_uint(),
            value.to_long(),
            value.to_ulong(),
            value.to_int64(),
            value.to_uint64(),
            f"{value.to_double():.6f}",
            value.to_signed(),
            value.to_unsigned(),
        )
        emit(
            family,
            "observe",
            "reductions",
            int(value.and_reduce()),
            int(value.nand_reduce()),
            int(value.or_reduce()),
            int(value.nor_reduce()),
            int(value.xor_reduce()),
            int(value.xnor_reduce()),
        )
        emit(family, "observe", "text", value.to_string(dt.SC_BIN), value.to_string(dt.SC_HEX, True))
        printed = cppyy.gbl.std.ostringstream()
        value.print(printed)
        emit(family, "observe", "print", printed.str())
        dumped = cppyy.gbl.std.ostringstream()
        value.dump(dumped)
        emit(family, "observe", "dump", dumped.str())

        count = probe.digit_count(value.length())
        packed = digit_array(count)
        value.get_packed_rep(packed.data())
        emit(family, "storage", "packed", probe.digit_text(packed.data(), count))
        raw = value.get_raw()
        emit(family, "storage", "raw", probe.digit_text(raw, count))
        for index in range(count):
            packed[index] = 0
        packed[0] = 0x5A
        value.set_packed_rep(packed.data())
        emit_value(family, "storage", "set_packed", value)

        value.__assign__(0)
        value.set(5)
        value.set(9, True)
        emit_value(family, "bits", "set", value)
        value.clear(5)
        value.invert(7)
        emit(family, "bits", "clear_invert", decimal(value), int(value.test(7)))
        value.reverse()
        emit_value(family, "bits", "reverse", value)

        value.__assign__(0x12345)
        value[7].__assign__(True)
        value.bit(8).__assign__(False)
        read_bit = probe.const_signed_bit(value, 7) if signed else probe.const_unsigned_bit(value, 7)
        emit(family, "view", "bits", int(value.test(7)), int(read_bit.to_bool()))
        value.range(15, 8).__assign__(0xA5)
        emit(family, "view", "mutable_range", decimal(value), value.range(15, 8).length())
        value(15, 8).__assign__(0x3C)
        emit(family, "view", "mutable_call", decimal(value), value(15, 8).length())
        view = probe.const_signed_range(value, 15, 8) if signed else probe.const_unsigned_range(value, 15, 8)
        emit(family, "view", "const_range", decimal(owner_type(view)), view.length())
        call = probe.const_signed_call(value, 15, 8) if signed else probe.const_unsigned_call(value, 15, 8)
        emit(family, "view", "const_call", decimal(owner_type(call)), call.length())

        controls = digit_array(count)
        data = digit_array(count)
        has_xz = cppyy.gbl.std.array[bool, 1]()
        has_xz[0] = False
        bit_count = value.concat_length(has_xz.data())
        control_present = value.concat_get_ctrl(controls.data(), 0)
        data_present = value.concat_get_data(data.data(), 0)
        emit(
            family,
            "concat",
            "get",
            bit_count,
            int(has_xz[0]),
            int(control_present),
            probe.digit_text(controls.data(), count),
            int(data_present),
            probe.digit_text(data.data(), count),
            value.concat_get_uint64(),
        )
        value.__assign__(0)
        value.concat_set(probe.SignedScalar(-5), 4)
        emit_value(family, "concat", "set_int64", value)
        value.__assign__(0)
        value.concat_set(probe.UnsignedScalar(11), 4)
        emit_value(family, "concat", "set_uint64", value)
        signed_source = dt.sc_signed(16)
        signed_source.__assign__(-5)
        unsigned_source = dt.sc_unsigned(16)
        unsigned_source.__assign__(11)
        value.__assign__(0)
        value.concat_set(signed_source, 4)
        emit_value(family, "concat", "set_signed", value)
        value.__assign__(0)
        value.concat_set(unsigned_source, 4)
        emit_value(family, "concat", "set_unsigned", value)

        for width in (1, 32, 137, 191, 257):
            wide = owner_type(width)
            wide.__assign__(scalar)
            emit_value(family, "width", width, wide)

        stream = cppyy.gbl.std.istringstream("-12345")
        value.scan(stream)
        emit(family, "stream", "scan", decimal(value), int(stream.rdstate()))


def exercise_errors():
    """Check native error reports for invalid indices, ranges and widths."""
    for family, owner_type in (("sc_dt::sc_signed", dt.sc_signed), ("sc_dt::sc_unsigned", dt.sc_unsigned)):
        value = owner_type(32)
        value.__assign__(7)
        previous = probe.throw_errors()
        try:
            for index in (-1, 32):
                try:
                    value.check_index(index)
                except cppyy.gbl.sc_core.sc_report as report:
                    emit(family, "error", f"index_{index}", report.get_msg_type(), report.get_msg())
                try:
                    value.invalid_index(index)
                except cppyy.gbl.sc_core.sc_report as report:
                    emit(family, "error", f"invalid_index_{index}", report.get_msg_type(), report.get_msg())
            try:
                value.check_range(32, 0)
            except cppyy.gbl.sc_core.sc_report as report:
                emit(family, "error", "range_32_0", report.get_msg_type(), report.get_msg())
            try:
                value.invalid_range(32, 0)
            except cppyy.gbl.sc_core.sc_report as report:
                emit(family, "error", "invalid_range_32_0", report.get_msg_type(), report.get_msg())
        finally:
            probe.restore_errors(previous)

        previous = probe.throw_errors()
        try:
            for width in (0, -1):
                try:
                    owner_type(width)
                except cppyy.gbl.sc_core.sc_report as report:
                    emit(family, "error", f"width_{width}", 1, report.get_msg_type(), report.get_msg())
                except TypeError as error:
                    report_marker = "sc_report: Error: (E400) "
                    diagnostic = str(error)
                    if report_marker not in diagnostic:
                        raise
                    report = diagnostic.split(report_marker, 1)[1].split("\n", 1)[0]
                    report_type, message = report.split(": ", 1)
                    emit(family, "error", f"width_{width}", 1, report_type, message)
        finally:
            probe.restore_errors(previous)


PHASES = {
    "constructors": exercise_constructors,
    "compounds": exercise_compounds,
    "storage": exercise_storage,
    "errors": exercise_errors,
}
if len(sys.argv) != 2 or sys.argv[1] not in PHASES:
    raise ValueError(f"expected one phase from {tuple(PHASES)}")
PHASES[sys.argv[1]]()
print("SC_DT_INTEGER_OWNERS_PUBLIC_OK")
