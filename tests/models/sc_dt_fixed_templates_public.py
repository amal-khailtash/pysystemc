"""Compare all four fixed-template declaration families with exact native calls."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_fixed_templates_probe.h"))
probe = cppyy.gbl.fixed_templates_probe


def emit(operation, family, label, *values):
    """Emit one constructor, mutation, policy, error or lifetime observation."""
    print("FTM\t" + "\t".join(map(str, (operation, family, label, *values))))


def render(value):
    """Copy the live number's exact decimal value, policies and cast flags."""
    return "\t".join(
        map(
            str,
            (
                value.to_string(dt.SC_DEC),
                value.wl(),
                value.iwl(),
                int(value.q_mode()),
                int(value.o_mode()),
                value.n_bits(),
                int(value.cast_switch() == dt.SC_ON),
                int(value.quantization_flag()),
                int(value.overflow_flag()),
            ),
        )
    )


def construct(number_type, signature, *arguments):
    """Initialize independent native storage through an exact public constructor."""
    number = number_type.__new__(number_type)
    number_type.__init__.__overload__(signature)(number, *arguments)
    assert number.__python_owns__
    return number


def check_result(operation, family, label, number, result):
    """Verify a mutation returns the same concrete specialization reference."""
    emit(
        operation,
        family,
        label,
        render(number),
        int(cppyy.addressof(result) == cppyy.addressof(number)),
        int(type(result) is type(number)),
    )
    assert cppyy.addressof(result) == cppyy.addressof(number)
    assert type(result) is type(number), "fixed-template mutation returned an inherited base type"


def sources():
    """Return every declared scalar, native FX and integer source with exact signatures."""
    unsigned_int = probe.UnsignedInt(0xA5A55A5A)
    long_value = -0x123456789ABCDEF if cppyy.sizeof("long") >= 8 else -0x1234567
    signed_long = probe.Long(long_value)
    unsigned_long_value = 0x123456789ABCDEF if cppyy.sizeof("unsigned long") >= 8 else 0x12345678
    unsigned_long = probe.UnsignedLong(unsigned_long_value)
    native_float = probe.Float(1.375)
    for value, expected, alias in (
        (unsigned_int, "unsigned int", "UnsignedInt"),
        (signed_long, "long", "Long"),
        (unsigned_long, "unsigned long", "UnsignedLong"),
        (native_float, "float", "Float"),
    ):
        spelling = getattr(type(value), "__cpp_name__", "")
        assert spelling in {expected, f"fixed_templates_probe::{alias}"}, spelling
        assert type(value) is getattr(probe, alias)
        assert cppyy.sizeof(type(value)) == cppyy.sizeof(expected)
    assert (
        int(unsigned_int) == 0xA5A55A5A and int(signed_long) == long_value and int(unsigned_long) == unsigned_long_value
    )
    assert float(native_float) == 1.375
    return (
        ("int", "int", -3),
        ("unsigned_int", "unsigned int", unsigned_int),
        ("long", "long", signed_long),
        ("unsigned_long", "unsigned long", unsigned_long),
        ("float", "float", native_float),
        ("double", "double", 2.625),
        ("text", "const char*", "3.1875"),
        ("fxval", "const sc_dt::sc_fxval&", probe.fxval_source()),
        ("fxval_fast", "const sc_dt::sc_fxval_fast&", probe.fxval_fast_source()),
        ("fxnum", "const sc_dt::sc_fxnum&", probe.fxnum_source()),
        ("fxnum_fast", "const sc_dt::sc_fxnum_fast&", probe.fxnum_fast_source()),
        ("int64", "sc_dt::int64", dt.int64(-0x123456789ABCDEF)),
        ("uint64", "sc_dt::uint64", dt.uint64(0xFEDCBA9876543210)),
        ("int_base", "const sc_dt::sc_int_base&", probe.int_base_source()),
        ("uint_base", "const sc_dt::sc_uint_base&", probe.uint_base_source()),
        ("signed", "const sc_dt::sc_signed&", probe.signed_source()),
        ("unsigned", "const sc_dt::sc_unsigned&", probe.unsigned_source()),
    )


def source_case(family, number_type, label, signature, source, observer, observer_signature):
    """Cover both constructor shapes and typed assignment/arithmetic returns."""
    off = dt.sc_fxcast_switch(dt.SC_OFF)
    number = construct(number_type, f"{signature}, {observer_signature}", source, observer)
    emit("ctor", family, label, render(number), int(probe.observer_matches(number, observer)))
    del number
    number = construct(
        number_type, f"{signature}, const sc_dt::sc_fxcast_switch&, {observer_signature}", source, off, observer
    )
    emit("ctor_cast", family, label, render(number), int(probe.observer_matches(number, observer)))
    del number
    number = number_type(source, observer)
    emit("ctor_public", family, label, render(number), int(probe.observer_matches(number, observer)))
    assert probe.observer_matches(number, observer), "public constructor lost its borrowed observer"
    del number
    number = number_type(source, off, observer)
    emit("ctor_public_cast", family, label, render(number), int(probe.observer_matches(number, observer)))
    assert probe.observer_matches(number, observer)
    del number
    number = number_type("37.8125")
    # Exact selectors establish declaration availability; public calls verify dispatch.
    number.__assign__.__overload__(signature)
    result = number.__assign__(source)
    check_result("assign", family, label, number, result)
    del result
    for operation, method in (
        ("add", "__iadd__"),
        ("subtract", "__isub__"),
        ("multiply", "__imul__"),
        ("divide", "__itruediv__"),
    ):
        number.__assign__("37.8125")
        getattr(number, method).__overload__(signature)
        result = getattr(number, method)(source)
        check_result(operation, family, label, number, result)
        del result
    del number


def run_family(family, number_type, is_fast, is_signed):
    """Exercise all 137 declaration rows plus native observer and copy lifetimes."""
    observer_type = probe.FastObserver if is_fast else probe.Observer
    observer_signature = "sc_dt::sc_fxnum_fast_observer*" if is_fast else "sc_dt::sc_fxnum_observer*"
    mask_type = dt.sc_fix if is_signed else dt.sc_ufix
    fast_mask_type = dt.sc_fix_fast if is_signed else dt.sc_ufix_fast
    postfix_type = dt.sc_fxval_fast if is_fast else dt.sc_fxval
    traits = probe.Signatures[number_type, mask_type, fast_mask_type, postfix_type]
    emit("traits", family, "exact", int(traits.valid()), int(probe.observers_enabled()))
    before = observer_type.destroyed
    observer = observer_type()
    off = dt.sc_fxcast_switch(dt.SC_OFF)
    number = construct(number_type, observer_signature, observer)
    emit("ctor", family, "observer", render(number), int(probe.observer_matches(number, observer)))
    del number
    number = construct(number_type, f"const sc_dt::sc_fxcast_switch&, {observer_signature}", off, observer)
    emit("ctor", family, "cast_observer", render(number), int(probe.observer_matches(number, observer)))
    del number
    source = construct(number_type, f"const char*, {observer_signature}", "7.875", observer)
    copied = number_type(source)
    emit("ctor", family, "copy", render(copied), int(probe.observer_matches(copied, cppyy.nullptr)))
    del source, copied
    for label, signature, source in sources():
        source_case(family, number_type, label, signature, source, observer, observer_signature)
    del source
    number, source = number_type("37.8125"), number_type("7.875")
    result = number.__assign__(source)
    check_result("assign", family, "copy", number, result)
    del source, result
    mask = mask_type("21.25", 19, 7)
    fast_mask = fast_mask_type("17.875", 19, 7)
    for operation, method in (("and", "__iand__"), ("or", "__ior__"), ("xor", "__ixor__")):
        for label, source in (("slow", mask), ("fast", fast_mask)):
            number.__assign__("37.8125")
            getattr(number, method).__overload__(f"const {type(source).__cpp_name__}&")
            result = getattr(number, method)(source)
            check_result(operation, family, label, number, result)
            del result
    for operation, method, count in (("left", "__ilshift__", 2), ("right", "__irshift__", 3)):
        number.__assign__("37.8125")
        getattr(number, method).__overload__("int")
        result = getattr(number, method)(count)
        check_result(operation, family, "int", number, result)
        del result
    number.__assign__("3.875")
    result = number.__preinc__()
    check_result("preinc", family, "none", number, result)
    del result
    previous = number.__postinc__(0)
    emit("postinc", family, "int", previous.to_string(), render(number), int(type(previous) is postfix_type))
    assert type(previous) is postfix_type and previous.__python_owns__
    del previous
    result = number.__predec__()
    check_result("predec", family, "none", number, result)
    del result
    previous = number.__postdec__(0)
    emit("postdec", family, "int", previous.to_string(), render(number), int(type(previous) is postfix_type))
    assert type(previous) is postfix_type and previous.__python_owns__
    del previous, number, mask, fast_mask, source, off
    gc.collect()
    emit("observer_lifetime", family, "numbers_gone", int(observer_type.destroyed == before))
    del observer
    gc.collect()
    emit("observer_lifetime", family, "observer_gone", int(observer_type.destroyed == before + 1))
    copy, postfix = number_type("0"), postfix_type("0")
    source = number_type("7.875")
    constructor_copy = number_type(source)
    copy.__assign__(source)
    postfix.__assign__(source.__postinc__(0))
    source.__assign__("0")
    del source
    gc.collect()
    emit("lifetime", family, "copy_postfix", render(copy), postfix.to_string(), render(constructor_copy))
    handler = cppyy.gbl.sc_core.sc_report_handler
    actions = handler.set_actions(cppyy.gbl.sc_core.SC_ERROR, cppyy.gbl.sc_core.SC_THROW)
    constructor_rejected = assignment_rejected = False
    try:
        try:
            number_type("invalid-number")
        except (cppyy.gbl.sc_core.sc_report, TypeError) as error:
            assert "invalid fixed-point value" in str(error)
            constructor_rejected = True
        try:
            copy.__assign__("invalid-number")
        except (cppyy.gbl.sc_core.sc_report, TypeError) as error:
            assert "invalid fixed-point value" in str(error)
            assignment_rejected = True
    finally:
        handler.set_actions(cppyy.gbl.sc_core.SC_ERROR, actions)
    emit("error", family, "text", int(constructor_rejected), int(assignment_rejected))
    assert constructor_rejected and assignment_rejected


def policy(family, number_type, label, initial, assigned):
    """Compare configured quantization, overflow and native fast precision limits."""
    value = number_type(initial)
    emit("policy_ctor", family, label, render(value))
    value.__assign__(assigned)
    emit("policy_assign", family, label, render(value))


for family, template, is_fast, is_signed in (
    ("sc_fixed", dt.sc_fixed, False, True),
    ("sc_ufixed", dt.sc_ufixed, False, False),
    ("sc_fixed_fast", dt.sc_fixed_fast, True, True),
    ("sc_ufixed_fast", dt.sc_ufixed_fast, True, False),
):
    run_family(family, template[19, 7], is_fast, is_signed)
for family, template in (
    ("sc_fixed", dt.sc_fixed),
    ("sc_ufixed", dt.sc_ufixed),
    ("sc_fixed_fast", dt.sc_fixed_fast),
    ("sc_ufixed_fast", dt.sc_ufixed_fast),
):
    policy(family, template[137, 100], "wide137", "9007199254740992.25", "9007199254740993.125")
    policy(family, template[191, 140], "wide191", "18014398509481984.25", "18014398509481985.125")
    policy(family, template[8, 4, dt.SC_RND_CONV, dt.SC_SAT, 0], "round_saturate", "1.96875", "19.75")
    policy(family, template[8, 4, dt.SC_TRN_ZERO, dt.SC_WRAP, 2], "truncate_wrap", "-9.9375", "19.78125")
    policy(family, template[17, -3], "negative_iwl", "0.015625", "-0.00390625")
    policy(family, template[17, 25], "integer_iwl", "1025", "-2049")
for family, template in (
    ("sc_fixed", dt.sc_fixed),
    ("sc_ufixed", dt.sc_ufixed),
    ("sc_fixed_fast", dt.sc_fixed_fast),
    ("sc_ufixed_fast", dt.sc_ufixed_fast),
):
    value = template[137, 100]((1 << 90) + 0x13579)
    emit("python_integer", family, "exact", render(value))
print("SC_DT_FIXED_TEMPLATES_PUBLIC_OK")
