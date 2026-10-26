"""Exercise all exact public Python comparison signatures in independent phases."""

import csv
import gc
import operator
from pathlib import Path
import re
import sys

from amal.eda.systemc import sc_dt as dt
import cppyy

ROOT = Path(__file__).resolve().parents[2]
OPERATIONS = {
    "lt": operator.lt,
    "le": operator.le,
    "gt": operator.gt,
    "ge": operator.ge,
    "eq": operator.eq,
    "ne": operator.ne,
}
FAMILIES = {
    "lt": "sc_dt::operator<",
    "le": "sc_dt::operator<=",
    "gt": "sc_dt::operator>",
    "ge": "sc_dt::operator>=",
    "eq": "sc_dt::operator==",
    "ne": "sc_dt::operator!=",
}
cppyy.include(str(ROOT / "tests/native/sc_dt_comparisons_probe.h"))
probe = cppyy.gbl.sc_dt_comparisons_probe


def signatures(family):
    """Return distinct declaration signatures in native lexical order."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="", encoding="utf-8") as source:
        return sorted(
            {
                row["signature"]
                for row in csv.DictReader(source)
                if row["name"] == family and row["kind"] == "FunctionDecl"
            }
        )


def split_arguments(signature):
    """Split exact C++ parameters without splitting nested template arguments."""
    text = signature[signature.index("(") + 1 : signature.rindex(")")]
    result, start, depth = [], 0, 0
    for index, character in enumerate(text):
        if character in "<(":
            depth += 1
        elif character in ">)":
            depth -= 1
        elif character == "," and depth == 0:
            result.append(text[start:index].strip())
            start = index + 1
    result.append(text[start:].strip())
    assert len(result) == 2, signature
    return result


def concrete_type(native_type):
    """Resolve dependent proxy aliases to the fixture's bv/lv representatives."""
    text = (
        native_type.replace("sc_proxy<X>", "sc_dt::sc_proxy<sc_dt::sc_bv_base>")
        .replace("sc_proxy<Y>", "sc_dt::sc_proxy<sc_dt::sc_lv_base>")
        .replace("sc_bitref_r<T1>", "sc_dt::sc_bitref_r<sc_dt::sc_bv_base>")
        .replace("sc_bitref_r<T2>", "sc_dt::sc_bitref_r<sc_dt::sc_lv_base>")
    )
    text = re.sub(r"(?<!sc_dt::)sc_proxy<", "sc_dt::sc_proxy<", text)
    return re.sub(r"(?<!sc_dt::)sc_bitref_r<", "sc_dt::sc_bitref_r<", text)


def python_operand(native_type, sources):
    """Return a live typed Python operand matching one native pointer parameter."""
    name = concrete_type(native_type).strip()
    if name.startswith("const "):
        name = name[6:]
    name = name.removesuffix("&").strip()
    if name == "bool":
        return probe.Bool(True)
    if name == "char":
        return probe.Char("1")
    if name == "int":
        return probe.SignedInt(1)
    if name == "unsigned int":
        return probe.UnsignedInt(0x12345)
    if name == "long":
        return probe.SignedLong(-1234567)
    if name == "unsigned long":
        return probe.UnsignedLong(0xABCDEF)
    if name == "float":
        return probe.Float(2.75)
    if name == "double":
        return -3.5
    if name == "sc_dt::int64":
        return probe.SignedScalar(-123456789)
    if name == "sc_dt::uint64":
        return probe.UnsignedScalar(0xFEDCBA9876543210)
    if name == "bool *":
        return sources.bool_bits.data()
    if name == "char *":
        return sources.char_bits.data()
    if name == "sc_dt::sc_logic *":
        return sources.logic_bits.data()
    if name == "sc_dt::sc_logic_value_t":
        return sources.logic_state
    members = {
        "sc_dt::sc_signed": "signed_value",
        "sc_dt::sc_unsigned": "unsigned_value",
        "sc_dt::sc_int_base": "signed_fixed",
        "sc_dt::sc_uint_base": "unsigned_fixed",
        "sc_dt::sc_fxnum": "fx_number",
        "sc_dt::sc_fxnum_fast": "fx_fast_number",
        "sc_dt::sc_fxval": "fx_value",
        "sc_dt::sc_fxval_fast": "fx_fast_value",
        "sc_dt::sc_fxnum_subref_r": "fx_subref",
        "sc_dt::sc_fxnum_fast_subref_r": "fx_fast_subref",
        "sc_dt::sc_bit": "bit_value",
        "sc_dt::sc_logic": "logic_value",
        "sc_dt::sc_concatref": "concat_value",
        "sc_dt::sc_fxcast_switch": "cast_switch",
        "sc_dt::sc_fxtype_params": "type_params",
        "sc_dt::sc_length_param": "length_param",
        "sc_dt::sc_bv_base": "bv",
        "sc_dt::sc_lv_base": "lv",
        "sc_dt::sc_proxy<sc_dt::sc_bv_base>": "bv",
        "sc_dt::sc_proxy<sc_dt::sc_lv_base>": "lv",
        "sc_dt::sc_bitref_r<sc_dt::sc_bv_base>": "bv_ref",
        "sc_dt::sc_bitref_r<sc_dt::sc_lv_base>": "lv_ref",
    }
    if name in members:
        return getattr(sources, members[name])
    raise AssertionError(f"no comparison operand factory for {native_type!r} -> {name!r}")


def run_phase(phase):
    """Invoke every unique public signature for one comparison operator."""
    sources = probe.Sources()
    operation = OPERATIONS[phase]
    for index, signature in enumerate(signatures(FAMILIES[phase])):
        left_type, right_type = split_arguments(signature)
        try:
            result = operation(python_operand(left_type, sources), python_operand(right_type, sources))
        except Exception as error:
            message = str(error).replace("\t", " ").replace("\n", " ")
            print(f"CMP\t{phase}\t{index}\tERROR:{type(error).__name__}:{message}", flush=True)
        else:
            print(f"CMP\t{phase}\t{index}\tPASS\t{int(bool(result))}", flush=True)


def run_representative_checks(sources):
    """Check precision, four-state width rules, invalid conversion and result lifetime."""
    low = dt.sc_fixed[128, 96]("9007199254740992")
    high = dt.sc_fixed[128, 96]("9007199254740993")
    precision = (operator.gt(high, low), operator.eq(high, low), operator.ne(high, low))
    print("EXTRA\tprecision\t" + "\t".join(str(int(value)) for value in precision), flush=True)

    left_logic = dt.sc_lv[4]("10XZ")
    same_logic = dt.sc_lv[4]("10XZ")
    changed_logic = dt.sc_lv[4]("10X0")
    short_bits = dt.sc_bv[4]("1010")
    long_bits = dt.sc_bv[8]("00001010")
    vector = (
        operator.eq(left_logic, same_logic),
        operator.eq(left_logic, changed_logic),
        operator.eq(short_bits, long_bits),
    )
    print("EXTRA\tvector\t" + "\t".join(str(int(value)) for value in vector), flush=True)

    lifetime_sources = probe.Sources()
    retained_result = bool(operator.eq(lifetime_sources.concat_value, lifetime_sources.signed_value))
    del lifetime_sources
    gc.collect()
    print(f"EXTRA\tlifetime\t{int(retained_result)}", flush=True)

    previous = probe.begin_invalid_value_throw()
    try:
        operator.eq(probe.Char("?"), sources.bit_value)
    except Exception as error:
        detail = str(error)
        report_id = int("E204" in detail)
        invalid_value = int("?" in detail)
    else:
        report_id = invalid_value = 0
    finally:
        probe.restore_invalid_value_action(previous)
    print(f"EXTRA\tinvalid\t{report_id}\t{invalid_value}\t{int(sources.bit_value.to_bool())}", flush=True)


def main():
    """Run one operator or all operators in this fresh cppyy process."""
    if len(sys.argv) != 2:
        raise SystemExit("expected lt, le, gt, ge, eq, ne, or all")
    phases = list(OPERATIONS) if sys.argv[1] == "all" else [sys.argv[1]]
    if any(phase not in OPERATIONS for phase in phases):
        raise SystemExit("unknown comparison phase")
    for phase in phases:
        run_phase(phase)
        if phase == "eq":
            run_representative_checks(probe.Sources())
    count = sum(len(signatures(FAMILIES[phase])) for phase in phases)
    print(f"CMP\tprobe_count\t{count}")
    print("SC_DT_COMPARISONS_PUBLIC_OK")


if __name__ == "__main__":
    main()
