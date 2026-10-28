"""Compare integer-template constructors and assignments with native SystemC."""

import gc
from pathlib import Path
import sys

from amal.eda.systemc import sc_dt as dt
import cppyy


cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_dt_integer_templates_probe.h"))
probe = cppyy.gbl.sc_dt_integer_templates_probe


def render(value):
    """Return a SystemC integer's full-width decimal value.

    Args:
        value: A native integer object that implements ``to_string``.

    Returns:
        Its exact decimal representation without narrowing to a machine word.
    """
    spelling = value.to_string(dt.SC_DEC)
    return spelling.decode() if isinstance(spelling, bytes) else str(spelling)


def emit(family, width, operation, value):
    """Print one constructor or assignment result for native comparison.

    Args:
        family: Native SystemC template family name.
        width: Instantiated integer width.
        operation: Constructor, assignment, lifecycle, or arithmetic API row.
        value: Scalar value or status field written after the operation label.
    """
    print("ITM\t" + "\t".join(map(str, (family, width, operation, value))))


def exercise_case(family, width, target_type, label, source):
    """Call the Python constructor and assignment for one native source type.

    Args:
        family: Native SystemC template family name.
        width: Instantiated integer width.
        target_type: Python type proxy for the target template specialization.
        label: Public overload label under test.
        source: Python proxy retaining the intended native source type.
    """
    constructed = target_type(source)
    emit(family, width, "ctor_" + label, render(constructed))
    assigned = target_type()
    result = assigned.__assign__(source)
    emit(family, width, "assign_" + label, render(assigned))
    emit(family, width, "assign_" + label + "_self", int(cppyy.addressof(result) == cppyy.addressof(assigned)))
    assert bool(constructed.__python_owns__)
    assert bool(assigned.__python_owns__)
    del constructed, assigned, result


def exercise_copy(family, width, target_type, initial_value):
    """Check value copying, assignment, owner independence, and destruction.

    Args:
        family: Native SystemC template family name.
        width: Instantiated integer width.
        target_type: Python type proxy for the target template specialization.
        initial_value: String value parsed by the target constructor.
    """
    default = target_type()
    emit(family, width, "default_ctor", render(default))
    del default
    initial = target_type(initial_value)
    copied = target_type(initial)
    assigned = target_type()
    assign_result = assigned.__assign__(initial)
    emit(family, width, "copy_ctor", render(copied))
    emit(family, width, "copy_assign", render(assigned))
    emit(
        family,
        width,
        "copy_assignment_self",
        int(cppyy.addressof(assign_result) == cppyy.addressof(assigned)),
    )
    initial.__assign__(0)
    emit(family, width, "copy_independent", render(copied))
    del initial, assigned, assign_result
    gc.collect()
    emit(family, width, "copy_lifetime", render(copied))
    copied.__destruct__()
    del copied
    gc.collect()


def exercise_width_probe(family, width, target_type):
    """Check that a nonstandard native width remains constructible and copyable.

    Args:
        family: Native SystemC family name.
        width: Instantiated width under test.
        target_type: Python type proxy for that exact width.
    """
    constructed = target_type("123456789")
    emit(family, width, "width_probe_ctor", render(constructed))
    assigned = target_type()
    result = assigned.__assign__(constructed)
    emit(family, width, "width_probe_assign", render(assigned))
    emit(family, width, "width_probe_assign_self", int(cppyy.addressof(result) == cppyy.addressof(assigned)))
    copied = target_type(constructed)
    emit(family, width, "width_probe_copy", render(copied))
    del constructed, assigned, result, copied
    gc.collect()


def integer_sources():
    """Create native source values used by the big integer overload matrix.

    Returns:
        Mapping of public source overload labels to live source values. Range
        and concatenation views remain valid because their vector/integer owners
        are retained in the returned source-owner list.
    """
    signed = dt.sc_signed(80)
    signed.__assign__("-123456789012")
    unsigned = dt.sc_unsigned(80)
    unsigned.__assign__("234567890123")
    signed_range = signed(31, 5)
    unsigned_range = unsigned(31, 5)
    bit_vector = dt.sc_bv[16]("1010010110100101")
    logic_vector = dt.sc_lv[16]("0110101011010011")
    generic = dt.concat(signed, unsigned)
    fixed_signed = dt.sc_int[23](-345678)
    fixed_unsigned = dt.sc_uint[23](456789)
    sources = {
        "signed": signed,
        "signed_subref": signed_range,
        "generic_concat": generic,
        "unsigned": unsigned,
        "unsigned_subref": unsigned_range,
        "string": "-765432109876",
        "int64": probe.int64_value(),
        "uint64": probe.uint64_value(),
        "long": probe.long_value(),
        "ulong": probe.ulong_value(),
        "int": probe.int_value(),
        "uint": probe.uint_value(),
        "double": probe.double_value(),
        "bv": bit_vector,
        "lv": logic_vector,
    }
    owners = [signed, unsigned, bit_vector, logic_vector, generic, fixed_signed, fixed_unsigned]
    return sources, owners


def fixed_point_sources():
    """Create values for all four SC_INCLUDE_FX overload families.

    Returns:
        Mapping of fixed-point overload labels to live native values.
    """
    return {
        "fxval": dt.sc_fxval(23.75),
        "fxval_fast": dt.sc_fxval_fast(31.5),
        "fxnum": dt.sc_fixed[20, 12](41.75),
        "fxnum_fast": dt.sc_fixed_fast[20, 12](57.25),
    }


def exercise_big(family, width, target_type):
    """Exercise arbitrary-width constructor and assignment overloads.

    Args:
        family: Native SystemC family name, either ``sc_bigint`` or
            ``sc_biguint``.
        width: Instantiated arbitrary width.
        target_type: Python type proxy for this exact width.
    """
    exercise_copy(family, width, target_type, "12345678901234567890")
    sources, owners = integer_sources()
    assignment_only = {"int_base": owners[-2], "uint_base": owners[-1]}
    for label, source in sources.items():
        exercise_case(family, width, target_type, label, source)
    for label, source in assignment_only.items():
        assigned = target_type()
        result = assigned.__assign__(source)
        emit(family, width, "assign_" + label, render(assigned))
        emit(family, width, "assign_" + label + "_self", int(cppyy.addressof(result) == cppyy.addressof(assigned)))
        del assigned, result
    for label, source in fixed_point_sources().items():
        exercise_case(family, width, target_type, label, source)

    scalar = probe.Int64(0x123456789AB) if family.endswith("sc_bigint") else probe.uint64_value()
    expected_gate = target_type(scalar)
    python_integer = int(scalar)
    python_constructed = target_type(python_integer)
    emit(family, width, "python_int_gate_expected", render(expected_gate))
    emit(family, width, "python_int_gate_ctor", render(python_constructed))
    assigned = target_type()
    assigned.__assign__(scalar)
    emit(family, width, "python_int_gate_assign_expected", render(assigned))
    assigned.__assign__(python_integer)
    emit(family, width, "python_int_gate_assign", render(assigned))
    keyword_constructed = target_type(v=python_integer)
    assert render(keyword_constructed) == render(python_constructed)
    keyword_generic = target_type(a=sources["generic_concat"])
    expected_generic = target_type(sources["generic_concat"])
    assert render(keyword_generic) == render(expected_generic)
    del keyword_constructed, keyword_generic, expected_generic
    del expected_gate, python_constructed, assigned
    gc.collect()
    owners.clear()
    del sources, assignment_only
    gc.collect()


def exercise_fixed(family, width, target_type, source_type):
    """Exercise bounded constructors, assignments, and arithmetic overloads.

    Args:
        family: Native SystemC family name, either ``sc_int`` or ``sc_uint``.
        width: Instantiated bounded width.
        target_type: Python type proxy for the target specialization.
        source_type: Distinct-width same-signedness type exposing the base
            constructor and assignment overloads.
    """
    exercise_copy(family, width, target_type, "74565")
    fixed_source = source_type(0x23456)
    fixed_range = (
        probe.const_int_range(fixed_source, 15, 3)
        if family.endswith("sc_int")
        else probe.const_uint_range(fixed_source, 15, 3)
    )
    signed_source = dt.sc_signed(48)
    signed_source.__assign__("-123456789012")
    unsigned_source = dt.sc_unsigned(48)
    unsigned_source.__assign__("234567890123")
    bit_vector = dt.sc_bv[16]("1010010110100101")
    logic_vector = dt.sc_lv[16]("0110101011010011")
    generic = dt.concat(signed_source, unsigned_source)
    sources = {
        "base": fixed_source,
        "subref": fixed_range,
        "generic_concat": generic,
        "signed": signed_source,
        "unsigned": unsigned_source,
        "string": "-765432109876",
    }
    is_signed = family.endswith("sc_int")
    if is_signed:
        sources["int_type"] = probe.int64_value()
        sources["uint64"] = probe.uint64_value()
    else:
        sources["uint_type"] = probe.uint64_value()
        sources["int64"] = probe.int64_value()
    sources.update(
        {
            "long": probe.long_value(),
            "ulong": probe.ulong_value(),
            "int": probe.int_value(),
            "uint": probe.uint_value(),
            "double": probe.double_value(),
            "bv": bit_vector,
            "lv": logic_vector,
        }
    )
    for label, source in sources.items():
        exercise_case(family, width, target_type, label, source)
    for label, source in fixed_point_sources().items():
        exercise_case(family, width, target_type, label, source)

    scalar = probe.Int64(0x123456789AB) if is_signed else probe.uint64_value()
    expected_gate = target_type(scalar)
    python_constructed = target_type(int(scalar))
    emit(family, width, "python_int_gate_expected", render(expected_gate))
    emit(family, width, "python_int_gate_ctor", render(python_constructed))
    assigned = target_type()
    assigned.__assign__(scalar)
    emit(family, width, "python_int_gate_assign_expected", render(assigned))
    assigned.__assign__(int(scalar))
    emit(family, width, "python_int_gate_assign", render(assigned))
    keyword_constructed = target_type(v=int(scalar))
    assert render(keyword_constructed) == render(python_constructed)
    keyword_generic = target_type(a=generic)
    expected_generic = target_type(generic)
    assert render(keyword_generic) == render(expected_generic)
    del keyword_constructed, keyword_generic, expected_generic
    exercise_operators(family, width, target_type, is_signed)
    del expected_gate, python_constructed, assigned, fixed_source
    del fixed_range, signed_source, unsigned_source, bit_vector, logic_vector, generic, sources
    gc.collect()


def exercise_operators(family, width, target_type, is_signed):
    """Exercise all arithmetic and bitwise assignment and increment methods.

    Args:
        family: Native SystemC family name.
        width: Instantiated bounded width.
        target_type: Python type proxy for the target specialization.
        is_signed: True for signed values, false for unsigned values.
    """
    value = target_type(48)
    scalar = probe.int64_value() if is_signed else probe.uint64_value()
    for operation, operand in (
        ("plus_assign", 5),
        ("minus_assign", 3),
        ("multiply_assign", 2),
        ("divide_assign", 4),
        ("modulo_assign", 7),
        ("and_assign", 6),
        ("or_assign", 8),
        ("xor_assign", 3),
        ("left_shift_assign", 2),
        ("right_shift_assign", 1),
    ):
        method = {
            "plus_assign": "__iadd__",
            "minus_assign": "__isub__",
            "multiply_assign": "__imul__",
            "divide_assign": "__itruediv__",
            "modulo_assign": "__imod__",
            "and_assign": "__iand__",
            "or_assign": "__ior__",
            "xor_assign": "__ixor__",
            "left_shift_assign": "__ilshift__",
            "right_shift_assign": "__irshift__",
        }[operation]
        getattr(value, method)(operand)
        emit(family, width, operation, render(value))
    prefix_up = value.__preinc__()
    emit(family, width, "prefix_increment", render(prefix_up))
    postfix_up = value.__postinc__(0)
    emit(family, width, "postfix_increment_old", render(postfix_up))
    emit(family, width, "postfix_increment_new", render(value))
    prefix_down = value.__predec__()
    emit(family, width, "prefix_decrement", render(prefix_down))
    postfix_down = value.__postdec__(0)
    emit(family, width, "postfix_decrement_old", render(postfix_down))
    emit(family, width, "postfix_decrement_new", render(value))
    del value, scalar, prefix_up, postfix_up, prefix_down, postfix_down
    gc.collect()


def exercise_invalid_strings(family, width, target_type):
    """Check that invalid constructor and assignment text raises SystemC errors.

    Args:
        family: Native SystemC family name.
        width: Instantiated integer width.
        target_type: Python type proxy for the target specialization.
    """
    old_actions = probe.throw_errors()
    try:
        try:
            target_type("")
        except Exception:
            emit(family, width, "invalid_string_ctor", 1)
        else:
            raise AssertionError(f"{family}[{width}] accepted empty constructor text")
        assigned = target_type()
        try:
            assigned.__assign__("")
        except Exception:
            emit(family, width, "invalid_string_assign", 1)
        else:
            raise AssertionError(f"{family}[{width}] accepted empty assignment text")
        del assigned
    finally:
        probe.restore_errors(old_actions)


def exercise_large_python_integer():
    """Compare a 4,800-digit Python integer without changing Python limits."""
    digit_limit = sys.get_int_max_str_digits()
    width = 16384
    target_type = dt.sc_biguint[width]
    huge_value = 1 << 16000
    expected = target_type()
    expected[16000].__assign__(True)
    constructed = target_type(huge_value)
    emit("sc_dt::sc_biguint", width, "python_large_int_ctor_expected", render(expected))
    emit("sc_dt::sc_biguint", width, "python_large_int_ctor", render(constructed))
    assigned = target_type()
    assigned.__assign__(huge_value)
    emit("sc_dt::sc_biguint", width, "python_large_int_assign_expected", render(expected))
    emit("sc_dt::sc_biguint", width, "python_large_int_assign", render(assigned))
    assert sys.get_int_max_str_digits() == digit_limit
    del expected, constructed, assigned, huge_value
    gc.collect()


for width in (48, 137, 191, 257):
    if width == 48:
        exercise_big("sc_dt::sc_bigint", width, dt.sc_bigint[width])
        exercise_big("sc_dt::sc_biguint", width, dt.sc_biguint[width])
    else:
        exercise_width_probe("sc_dt::sc_bigint", width, dt.sc_bigint[width])
        exercise_width_probe("sc_dt::sc_biguint", width, dt.sc_biguint[width])

for width in (19, 59, 63, 64):
    if width == 59:
        exercise_fixed("sc_dt::sc_int", width, dt.sc_int[width], dt.sc_int[17])
        exercise_fixed("sc_dt::sc_uint", width, dt.sc_uint[width], dt.sc_uint[17])
    else:
        exercise_width_probe("sc_dt::sc_int", width, dt.sc_int[width])
        exercise_width_probe("sc_dt::sc_uint", width, dt.sc_uint[width])

assert bool(probe.fixed_point_enabled()), "SC_INCLUDE_FX must expose the inventoried overloads"
exercise_large_python_integer()
exercise_invalid_strings("sc_dt::sc_bigint", 48, dt.sc_bigint[48])
exercise_invalid_strings("sc_dt::sc_biguint", 48, dt.sc_biguint[48])
exercise_invalid_strings("sc_dt::sc_int", 59, dt.sc_int[59])
exercise_invalid_strings("sc_dt::sc_uint", 59, dt.sc_uint[59])

print("SC_DT_INTEGER_TEMPLATES_PUBLIC_OK")
