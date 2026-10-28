"""Exercise generic SystemC vector proxies against an independent native model."""

from pathlib import Path

from amal.eda.systemc import sc_dt as dt
from amal.eda.systemc.sc_dt._operators import _vector_argument_type
import cppyy


NATIVE = Path(__file__).resolve().parents[1] / "native"
cppyy.include(str(NATIVE / "sc_dt_proxy_probe.h"))
probe = cppyy.gbl.sc_dt_proxy_probe
native_types = cppyy.gbl.sc_dt
pointer_operands = probe.PointerOperands()


def text(value):
    """Copy a native string-like result into ordinary Python text."""
    return value.decode() if isinstance(value, bytes) else str(value)


def bits(value):
    """Return a full-width native binary spelling while its owner is live."""
    return text(value.to_string())


def emit(family, group, label, *fields):
    """Write one stable row consumed by the native reference comparator."""
    print("SDP\t" + "\t".join(map(str, (family, group, label, *fields))))


def sources():
    """Build the 13 exact native source categories at full proxy width.

    Pointer sources refer to static buffers with 191 elements. They remain
    live for this process and satisfy the native vector pointer-length
    precondition; a caller using another pointer must retain equally wide
    storage for every operation.
    """
    unsigned = dt.sc_unsigned(191)
    unsigned.__assign__("0x123456789abcdef0123456789abcdef")
    signed = dt.sc_signed(191)
    signed.__assign__("-81985529216486895")
    uint_base = dt.sc_uint_base(63)
    uint_base.__assign__(dt.uint64(0x123456789ABCDEF0))
    int_base = dt.sc_int_base(63)
    int_base.__assign__(dt.int64(-0x123456789AB))
    return (
        # Python bit strings reach the production adapter as std::string and
        # are converted by operator_argument to the native const-char pointer.
        ("char_pointer", "const char*", "std::string", text(pointer_operands.char_pointer())),
        ("bool_pointer", "const bool*", "const bool*", pointer_operands.bool_pointer()),
        (
            "logic_pointer",
            "const sc_dt::sc_logic*",
            "const sc_dt::sc_logic*",
            pointer_operands.logic_pointer(),
        ),
        ("unsigned", "const sc_dt::sc_unsigned&", dt.sc_unsigned, unsigned),
        ("signed", "const sc_dt::sc_signed&", dt.sc_signed, signed),
        ("uint_base", "const sc_dt::sc_uint_base&", dt.sc_uint_base, uint_base),
        ("int_base", "const sc_dt::sc_int_base&", dt.sc_int_base, int_base),
        ("unsigned_long", "unsigned long", "unsigned long", 0x12345678),
        ("long", "long", "long", -0x13579),
        ("unsigned_int", "unsigned int", "unsigned int", 0xA53C),
        ("int", "int", "int", -0x5A35),
        ("uint64", "sc_dt::uint64", "sc_dt::uint64", dt.uint64(0xFEDCBA9876543210)),
        ("int64", "sc_dt::int64", "sc_dt::int64", dt.int64(-0x123456789ABCDE)),
    )


def source_case(family, owner, label, signature, source_type, source):
    """Call the same exact native binary/compound templates as production.

    The SystemC owner and all pointer storage remain live for each call. The
    template keeps C++ overload resolution exact while the public wrappers
    select these production operation helpers from the Python argument type.
    """
    vector_type = type(owner)
    operation = cppyy.gbl.pysystemc_datatypes.binary_operator[vector_type, source_type]
    and_value = operation.bit_and(owner, source)
    or_value = operation.bit_or(owner, source)
    xor_value = operation.bit_xor(owner, source)
    and_owner = vector_type(owner)
    or_owner = vector_type(owner)
    xor_owner = vector_type(owner)
    and_result = operation.compound_and(and_owner, source)
    or_result = operation.compound_or(or_owner, source)
    xor_result = operation.compound_xor(xor_owner, source)
    equal = operation.equal(owner, source)
    not_equal = operation.not_equal(owner, source)
    if _vector_argument_type(source) == source_type:
        # Exercise every production Python operator adapter whose runtime
        # operand maps to the exact native source type used by this case.
        assert bits(native_types.sc_lv_base(owner & source)) == bits(and_value)
        assert bits(native_types.sc_lv_base(owner | source)) == bits(or_value)
        assert bits(native_types.sc_lv_base(owner ^ source)) == bits(xor_value)
        assert int(owner == source) == int(equal)
        assert int(owner != source) == int(not_equal)
        for method, expected in (
            ("__iand__", and_owner),
            ("__ior__", or_owner),
            ("__ixor__", xor_owner),
        ):
            public_target = vector_type(owner)
            public_result = getattr(public_target, method)(source)
            assert bits(public_target) == bits(expected)
            assert cppyy.addressof(public_result) == cppyy.addressof(public_target)
    emit(
        family,
        "source",
        label,
        bits(and_value),
        bits(or_value),
        bits(xor_value),
        int(equal),
        int(not_equal),
        bits(and_owner),
        int(cppyy.addressof(and_result) == cppyy.addressof(and_owner)),
        bits(or_owner),
        int(cppyy.addressof(or_result) == cppyy.addressof(or_owner)),
        bits(xor_owner),
        int(cppyy.addressof(xor_result) == cppyy.addressof(xor_owner)),
    )


def source_matrix(family, owner, all_sources):
    """Exercise every typed proxy source while retaining pointer operands."""
    for label, signature, source_type, source in all_sources:
        source_case(family, owner, label, signature, source_type, source)


def member_matrix(family, owner):
    """Emit reductions, conversions, shifts, views, stream IO and mutations."""
    formatted = type(owner)(191)
    formatted.__assign__("101001001101011001011001110101011")
    stream = cppyy.gbl.std.ostringstream()
    owner.print(stream)
    emit(
        family,
        "observe",
        "initial",
        owner.length(),
        int(owner.and_reduce()),
        int(owner.nand_reduce()),
        int(owner.or_reduce()),
        int(owner.nor_reduce()),
        int(owner.xor_reduce()),
        int(owner.xnor_reduce()),
        int(owner.to_int()),
        int(owner.to_uint()),
        int(owner.to_long()),
        int(owner.to_ulong()),
        int(owner.to_int64()),
        int(owner.to_uint64()),
        text(owner.to_string()),
        text(formatted.to_string(dt.SC_BIN)),
        text(formatted.to_string(dt.SC_BIN, True)),
        text(stream.str()),
    )

    if family == "lv":
        handler = cppyy.gbl.sc_core.sc_report_handler
        report_id = cppyy.gbl.sc_core.SC_ID_INVALID_FX_VALUE_
        previous = handler.set_actions(report_id, cppyy.gbl.sc_core.SC_THROW)
        caught, report_type, message = 0, "", ""
        try:
            owner.to_string(dt.SC_BIN)
        except cppyy.gbl.sc_core.sc_report as report:
            caught = 1
            diagnostic = str(report)
            assert "(E305) invalid fixed-point value" in diagnostic
            report_type = text(report_id)
        finally:
            handler.set_actions(report_id, previous)
        emit(family, "format_report", "binary_with_xz", caught, report_type, message)

    left, right, reversed_value = type(owner)(owner), type(owner)(owner), type(owner)(owner)
    left_result = left.__ilshift__(67)
    right_result = right.__irshift__(71)
    reverse_result = reversed_value.reverse()
    complement = ~owner
    emit(
        family,
        "transform",
        "shift_reverse",
        bits(left),
        int(cppyy.addressof(left_result) == cppyy.addressof(left)),
        bits(right),
        int(cppyy.addressof(right_result) == cppyy.addressof(right)),
        bits(reversed_value),
        int(cppyy.addressof(reverse_result) == cppyy.addressof(reversed_value)),
        bits(complement),
    )

    left_rotation, right_rotation = type(owner)(owner), type(owner)(owner)
    left_result = left_rotation.lrotate(43)
    right_result = right_rotation.rrotate(53)
    emit(
        family,
        "transform",
        "rotate",
        bits(left_rotation),
        int(cppyy.addressof(left_result) == cppyy.addressof(left_rotation)),
        bits(right_rotation),
        int(cppyy.addressof(right_result) == cppyy.addressof(right_rotation)),
    )

    bit = owner.bit(23)
    index = owner[161]
    view = owner.range(151, 73)
    const_owner = type(owner)(owner)
    read_bit = const_owner.bit(23)
    read_index = const_owner[161]
    read_view = const_owner.range(151, 73)
    copied_view = type(view)(view)
    emit(
        family,
        "view",
        "selection",
        bits(native_types.sc_lv_base(bit)),
        bits(native_types.sc_lv_base(index)),
        bits(native_types.sc_lv_base(view)),
        bits(native_types.sc_lv_base(read_bit)),
        bits(native_types.sc_lv_base(read_index)),
        bits(native_types.sc_lv_base(read_view)),
        view.length(),
        read_view.length(),
    )
    view.__assign__("0x123456789abcdef")
    emit(family, "view", "write_through", bits(owner))
    del view
    emit(
        family,
        "view_copy",
        "after_shell_write",
        bits(native_types.sc_lv_base(copied_view)),
        copied_view.length(),
    )

    scan_input = cppyy.gbl.std.istringstream("101101001011")
    scanned = type(owner)(191)
    scanned.scan(scan_input)
    emit(family, "io", "scan", bits(scanned), int(scan_input.good()))


def custom_proxy_case():
    """Exercise CRTP method dispatch and virtual destruction on custom owner."""
    custom_type = probe.CustomProxy
    owner = custom_type("1010010010110101")
    source = custom_type("1100101010011010")
    assigned = owner.assign_(source)
    operation = cppyy.gbl.pysystemc_datatypes.binary_operator[custom_type, custom_type]
    expression = operation.bit_xor(owner, source)
    assert bits(owner ^ source) == bits(expression)
    assert bool(owner == source)
    compound_owner = custom_type("1010010010110101")
    compound_result = compound_owner.__ixor__(source)
    expected_owner = custom_type("1010010010110101")
    expected_result = operation.compound_xor(expected_owner, source)
    assert bits(compound_owner) == bits(expected_owner)
    assert cppyy.addressof(compound_result) == cppyy.addressof(compound_owner)
    assert cppyy.addressof(expected_result) == cppyy.addressof(expected_owner)
    emit(
        "custom",
        "crtp",
        "generic",
        owner.length(),
        bits(owner.storage()),
        bits(expression),
        int(cppyy.addressof(assigned) == cppyy.addressof(owner)),
        int(owner.xor_reduce()),
        int(owner.to_int()),
        text(owner.to_string()),
    )
    emit("custom", "lifetime", "virtual_delete", probe.virtual_destruction_count())


def report_case():
    """Record the configured native report from a negative proxy shift."""
    report_handler = cppyy.gbl.sc_core.sc_report_handler
    previous = report_handler.set_actions(cppyy.gbl.sc_core.SC_ID_OUT_OF_BOUNDS_, cppyy.gbl.sc_core.SC_THROW)
    owner = dt.sc_bv_base("101101")
    caught, report_type, message = 0, "", ""
    try:
        owner.__ilshift__(-3)
    except cppyy.gbl.sc_core.sc_report as report:
        caught = 1
        message = "left shift operation is only allowed with positive shift values, shift value = -3"
        diagnostic = str(report)
        assert "(E5) out of bounds" in diagnostic
        assert message in diagnostic
        report_type = text(cppyy.gbl.sc_core.SC_ID_OUT_OF_BOUNDS_)
    finally:
        report_handler.set_actions(cppyy.gbl.sc_core.SC_ID_OUT_OF_BOUNDS_, previous)
    emit("bv", "report", "negative_shift", caught, report_type, message, bits(owner))


all_sources = sources()
for _label, _signature, _source_type, _source in all_sources[:3]:
    assert _vector_argument_type(_source) == _source_type, _label
for family, vector_type, initial in (
    ("bv", dt.sc_bv_base, "101001001101011001011001110101011"),
    ("lv", dt.sc_lv_base, "10XZ1001101011001011001110101011"),
):
    owner = vector_type(191)
    owner.__assign__(initial)
    source_matrix(family, owner, all_sources)
    member_matrix(family, owner)

custom_proxy_case()
generic_owner = probe.CustomProxy(191)
generic_owner.assign_("101001001101011001011001110101011")
source_matrix("custom", generic_owner, all_sources)
report_case()
