"""Exercise Python dispatch for native integer and four-state binary operators."""

import csv
import gc
import operator
from pathlib import Path
import sys

from amal.eda.systemc import sc_dt as dt
import cppyy


ROOT = Path(__file__).resolve().parents[2]
cppyy.include(str(ROOT / "tests/native/sc_dt_integer_binary_probe.h"))
probe = cppyy.gbl.sc_dt_integer_binary_probe


FAMILIES = {
    "mod": "sc_dt::operator%",
    "and": "sc_dt::operator&",
    "or": "sc_dt::operator|",
    "xor": "sc_dt::operator^",
}


def signatures(family):
    """Return sorted unique declaration signatures for one exact operator family."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="", encoding="utf-8") as source:
        rows = csv.DictReader(source)
        return sorted({row["signature"] for row in rows if row["name"] == family and row["kind"] == "FunctionDecl"})


def split_arguments(signature):
    """Split the two C++ argument types while respecting template commas."""
    text = signature[signature.index("(") + 1 : signature.rindex(")")]
    result = []
    start = 0
    depth = 0
    for index, character in enumerate(text):
        if character in "<(":
            depth += 1
        elif character in ">)":
            depth -= 1
        elif character == "," and depth == 0:
            result.append(text[start:index].strip())
            start = index + 1
    result.append(text[start:].strip())
    assert len(result) == 2
    return result


def concrete_type(native_type):
    """Choose the explicit bv/lv proxy representative for dependent types."""
    return (
        native_type.replace("sc_proxy<X>", "sc_dt::sc_proxy<sc_dt::sc_bv_base>")
        .replace("sc_proxy<Y>", "sc_dt::sc_proxy<sc_dt::sc_lv_base>")
        .replace("sc_bitref_r<T1>", "sc_dt::sc_bitref_r<sc_dt::sc_bv_base>")
        .replace("sc_bitref_r<T2>", "sc_dt::sc_bitref_r<sc_dt::sc_lv_base>")
        .replace("sc_proxy<sc_dt::", "sc_dt::sc_proxy<sc_dt::")
    )


def python_operand(native_type, values):
    """Build a typed operand from the live width/sign-sensitive source bank."""
    native_type = concrete_type(native_type).strip()
    if native_type.startswith("const "):
        native_type = native_type[len("const ") :]
    native_type = native_type.removesuffix("&").strip()
    if native_type == "bool":
        return probe.Bool(True)
    if native_type == "char":
        return probe.Char("1")
    if native_type == "int":
        return probe.SignedInt(1)
    if native_type == "unsigned int":
        return probe.UnsignedInt(29)
    if native_type == "long":
        return probe.SignedLong(12345)
    if native_type == "unsigned long":
        return probe.UnsignedLong(65535)
    if native_type == "sc_dt::int64":
        return probe.SignedScalar(-123456789)
    if native_type == "sc_dt::uint64":
        return probe.UnsignedScalar(0xFEDCBA9876543210)
    if native_type == "sc_dt::sc_logic_value_t":
        return values.logic_state
    if native_type == "bool *":
        return values.bool_bits.data()
    if native_type == "char *":
        return values.char_bits.data()
    if native_type == "sc_dt::sc_logic *":
        return values.logic_bits.data()
    if native_type == "sc_dt::sc_concatref":
        return values.concat_value
    if "sc_bitref_r<" in native_type:
        return values.bv_ref if "sc_bv_base" in native_type else values.lv_ref
    if "sc_proxy<" in native_type:
        return values.bv if "sc_bv_base" in native_type else values.lv
    attributes = {
        "sc_dt::sc_signed": "signed_value",
        "sc_dt::sc_unsigned": "unsigned_value",
        "sc_dt::sc_int_base": "signed_fixed",
        "sc_dt::sc_uint_base": "unsigned_fixed",
        "sc_dt::sc_fix": "fixed_value",
        "sc_dt::sc_fix_fast": "fixed_fast_value",
        "sc_dt::sc_ufix": "ufixed_value",
        "sc_dt::sc_ufix_fast": "ufixed_fast_value",
        "sc_dt::sc_bit": "bit_value",
        "sc_dt::sc_logic": "logic_value",
    }
    if native_type in attributes:
        return getattr(values, attributes[native_type])
    raise AssertionError(f"no Python operand factory for {native_type!r}")


def payload(value):
    """Render native bits or scalar text and preserve a result width."""
    if hasattr(value, "to_string"):
        cpp_name = getattr(type(value), "__cpp_name__", "")
        if "sc_lv" in cpp_name or "sc_bv" in cpp_name:
            text = value.to_string()
        else:
            text = value.to_string(dt.SC_BIN)
        if hasattr(value, "length"):
            return f"{text}:{value.length()}"
        if hasattr(value, "wl") and hasattr(value, "iwl"):
            return f"{text}:{value.wl()}:{value.iwl()}"
        return str(text)
    if hasattr(value, "to_char"):
        return str(value.to_char())
    return str(value)


def matrix_phase(phase):
    """Exercise every unique signature through Python's public binary protocol."""
    family = FAMILIES[phase]
    operation = {"mod": operator.mod, "and": operator.and_, "or": operator.or_, "xor": operator.xor}[phase]
    values = probe.Sources()
    for index, signature in enumerate(signatures(family)):
        left_type, right_type = split_arguments(signature)
        try:
            left = python_operand(left_type, values)
            right = python_operand(right_type, values)
            result = operation(left, right)
            output = payload(result)
        except Exception as error:
            detail = str(error).replace("\t", " ").replace("\n", " ")
            print(f"MATRIX\t{phase}\t{index}\tERROR:{type(error).__name__}:{detail}\t{signature}", flush=True)
        else:
            print(f"MATRIX\t{phase}\t{index}\tPASS\t{output}", flush=True)


def render(value):
    """Return a type-sensitive value description with native width where present."""
    value_type = type(value)
    cpp_name = getattr(value_type, "__cpp_name__", "")
    if hasattr(value, "to_string"):
        if "sc_signed" in cpp_name:
            kind = "S"
        elif "sc_unsigned" in cpp_name:
            kind = "U"
        elif "sc_bv" in cpp_name:
            kind = "B"
        elif "sc_lv" in cpp_name:
            kind = "V"
        else:
            kind = "N"
        if "sc_lv" in cpp_name or "sc_bv" in cpp_name:
            bits = value.to_string()
        else:
            bits = value.to_string(dt.SC_BIN)
        if hasattr(value, "length"):
            suffix = f":{value.length()}"
        elif hasattr(value, "wl") and hasattr(value, "iwl"):
            suffix = f":{value.wl()}:{value.iwl()}"
        else:
            suffix = ""
        return f"{kind}:{bits}{suffix}"
    if hasattr(value, "to_char"):
        return str(value.to_char())
    return str(value)


def emit(label, value):
    """Write one native-comparable observation."""
    print(f"BIN\t{label}\t{render(value)}", flush=True)


def integer_phase():
    """Compare modulo and integer/concatenation bitwise dispatch."""
    signed_left = dt.sc_signed(19)
    signed_left.__assign__(-12345)
    signed_right = dt.sc_signed(13)
    signed_right.__assign__(37)
    unsigned_value = dt.sc_unsigned(21)
    unsigned_value.__assign__(0x1A55A)
    fixed_signed = dt.sc_int_base(12)
    fixed_signed.__assign__(-311)
    fixed_unsigned = dt.sc_uint_base(15)
    fixed_unsigned.__assign__(0x4A31)

    cases = (
        ("remainder_signed_signed", operator.mod, signed_left, signed_right),
        ("remainder_signed_int", operator.mod, signed_left, 29),
        ("remainder_int_signed", operator.mod, 987, signed_right),
        ("remainder_unsigned_fixed", operator.mod, unsigned_value, fixed_unsigned),
        ("and_signed_unsigned", operator.and_, signed_left, unsigned_value),
        ("or_unsigned_signed", operator.or_, unsigned_value, signed_left),
        ("xor_fixed_signed", operator.xor, fixed_signed, signed_right),
        ("and_int_unsigned", operator.and_, 0x735A, unsigned_value),
        ("or_unsigned_int", operator.or_, unsigned_value, 0x20011),
        ("xor_int_signed", operator.xor, -901, signed_left),
    )
    for label, operation, left, right in cases:
        emit(label, operation(left, right))

    concat_high = dt.sc_signed(9)
    concat_high.__assign__(0x12F)
    concat_low = dt.sc_unsigned(7)
    concat_low.__assign__(0x53)
    # sc_concatref shells are borrowed pooled views. Build and consume each in
    # one expression while retaining both integer parents.
    emit("remainder_integer_concat", operator.mod(dt.concat(concat_high, concat_low), signed_right))
    emit("and_integer_concat", operator.and_(dt.concat(concat_high, concat_low), 0x1FF))
    emit("or_concat_integer", operator.or_(0x301, dt.concat(concat_high, concat_low)))
    emit("xor_concat_unsigned", operator.xor(dt.concat(concat_high, concat_low), unsigned_value))

    left = dt.sc_signed(11)
    left.__assign__(-77)
    right = dt.sc_signed(9)
    right.__assign__(6)
    owned = left % right
    del left, right
    gc.collect()
    emit("lifetime_owned_result", owned)


def vector_phase():
    """Compare genuine vector, bit-reference, logic and scalar bitwise cases."""
    bits = dt.sc_bv[8]("10110010")
    logic = dt.sc_lv[8]("10ZX0101")
    emit("and_bv_lv", bits & logic)
    emit("or_lv_bv", logic | bits)
    emit("xor_bv_int", bits ^ 0x35)
    emit("or_int_bv", 0x0F | bits)
    emit("xor_bitrefs", bits[0] ^ logic[1])

    bool_array = cppyy.gbl.std.array[bool, 8]()
    char_array = cppyy.gbl.std.array["char", 9]()
    logic_array = cppyy.gbl.std.array[dt.sc_logic, 8]()
    for index, (bit, char, state) in enumerate(
        zip((True, False, True, True, False, False, True, False), "10110010", "10ZX0101")
    ):
        bool_array[index] = bit
        char_array[index] = ord(char)
        logic_array[index] = dt.sc_logic(state)
    bool_pointer = bool_array.data()
    char_pointer = char_array.data()
    logic_pointer = logic_array.data()
    emit("and_bool_pointer_left", bool_pointer & bits)
    emit("and_bool_pointer_right", bits & bool_pointer)
    emit("or_char_pointer_left", char_pointer | bits)
    emit("or_char_pointer_right", bits | char_pointer)
    emit("xor_logic_pointer_left", logic_pointer ^ bits)
    emit("xor_logic_pointer_right", bits ^ logic_pointer)

    owned_bits = dt.sc_bv[8]("11001010")
    owned_logic = dt.sc_lv[8]("01ZX0110")
    owned_result = owned_bits & owned_logic
    del owned_bits, owned_logic
    gc.collect()
    emit("lifetime_owned_vector", owned_result)

    logic_value = dt.sc_logic("Z")
    emit("and_logic_bool", logic_value & True)
    emit("or_logic_char", logic_value | "0")
    bit_value = dt.sc_bit(True)
    emit("xor_bit_int", bit_value ^ 0)


def main():
    """Run one independent kernel-free operator phase."""
    if len(sys.argv) != 2:
        raise SystemExit("expected integer or vector phase")
    phase = sys.argv[1]
    if phase in FAMILIES:
        matrix_phase(phase)
    elif phase == "integer":
        integer_phase()
    elif phase == "vector":
        vector_phase()
    else:
        raise SystemExit(f"unknown phase: {phase}")
    print(f"BIN\tprobe_count\t{probe.exact_overload_count}")
    print("SC_DT_INTEGER_BINARY_PUBLIC_OK")


if __name__ == "__main__":
    main()
