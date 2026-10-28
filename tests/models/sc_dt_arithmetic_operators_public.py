"""Exercise public Python arithmetic protocols against exact native operators."""

import csv
import gc
import operator
from pathlib import Path
import sys

from amal.eda.systemc import sc_dt as dt
import cppyy

ROOT = Path(__file__).resolve().parents[2]
cppyy.include(str(ROOT / "tests/native/sc_dt_arithmetic_operators_probe.h"))
probe = cppyy.gbl.sc_dt_arithmetic_operators_probe

FAMILIES = ("sc_dt::operator+", "sc_dt::operator-", "sc_dt::operator*", "sc_dt::operator/")
PUBLIC_OPERATIONS = (operator.add, operator.sub, operator.mul, operator.truediv)
UNARY_OPERATIONS = (operator.pos, operator.neg, None, None)
SOURCE_VIEWS = {
    "const sc_dt::sc_fxnum &": "fxnum",
    "const sc_dt::sc_fxnum_fast &": "fxnum_fast",
    "const sc_dt::sc_fxval &": "fxval",
    "const sc_dt::sc_fxval_fast &": "fxval_fast",
    "const sc_dt::sc_int_base &": "sint",
    "const sc_dt::sc_uint_base &": "uint",
    "const sc_dt::sc_signed &": "signed_value",
    "const sc_dt::sc_unsigned &": "unsigned_value",
    "const sc_dt::sc_concatref &": "concat",
    "const sc_dt::sc_string_old &": "old_string",
    "bool": "boolean",
    "const char *": "text",
    "double": "real_double",
    "float": "real_float",
    "int": "integer",
    "long": "long_integer",
    "sc_dt::int64": "signed_64",
    "sc_dt::uint64": "unsigned_64",
    "unsigned int": "unsigned_integer",
    "unsigned long": "unsigned_long_integer",
}
SCALAR_CONSTRUCTORS = {
    "bool": "Bool",
    "double": "RealDouble",
    "float": "RealFloat",
    "int": "SignedInt",
    "long": "SignedLong",
    "sc_dt::int64": "Signed64",
    "sc_dt::uint64": "Unsigned64",
    "unsigned int": "UnsignedInt",
    "unsigned long": "UnsignedLong",
}


def inventory_signatures(family):
    """Return sorted unique exact signatures for one binary or unary family."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="", encoding="utf-8") as source:
        return sorted(
            {
                row["signature"]
                for row in csv.DictReader(source)
                if row["name"] == family and row["kind"] == "FunctionDecl"
            }
        )


def split_arguments(signature):
    """Split a C++ signature at commas outside template argument lists."""
    text = signature[signature.index("(") + 1 : signature.rindex(")")]
    result = []
    depth = 0
    start = 0
    for index, character in enumerate(text):
        if character == "<":
            depth += 1
        elif character == ">":
            depth -= 1
        elif character == "," and depth == 0:
            result.append(text[start:index].strip())
            start = index + 1
    if text.strip():
        result.append(text[start:].strip())
    return result


def source_value(sources, spelling):
    """Return a live input with the declared SystemC base type where applicable."""
    method = SOURCE_VIEWS[spelling]
    if spelling in (
        "bool",
        "const char *",
        "double",
        "float",
        "int",
        "long",
        "sc_dt::int64",
        "sc_dt::uint64",
        "unsigned int",
        "unsigned long",
    ):
        value = getattr(probe.SourceView, method)()
        constructor = SCALAR_CONSTRUCTORS.get(spelling)
        return getattr(probe, constructor)(value) if constructor else value
    return getattr(probe.SourceView, method)(sources)


def render(value, expected_type):
    """Serialize full native precision and attach the concrete result type."""
    actual_type = getattr(type(value), "__cpp_name__", "")
    if actual_type != expected_type:
        raise AssertionError(f"operator returned {actual_type!r}, expected {expected_type!r}")
    if expected_type in ("sc_dt::sc_fxval", "sc_dt::sc_fxval_fast"):
        payload = "|".join(
            (
                format(float(value.to_double()), ".17g"),
                str(value.to_string(dt.SC_DEC)),
                str(int(value.is_neg())),
                str(int(value.is_zero())),
                str(int(value.is_nan())),
                str(int(value.is_inf())),
                str(int(value.is_normal())),
                str(int(value.rounding_flag())),
            )
        )
    elif expected_type in ("sc_dt::sc_signed", "sc_dt::sc_unsigned"):
        payload = f"{value.to_string(dt.SC_BIN)}:{int(value.length())}"
    elif expected_type == "sc_dt::sc_string_old":
        payload = str(value.c_str())
    else:
        raise AssertionError(f"no arithmetic result renderer for {expected_type!r}")
    owned = bool(value.__python_owns__)
    if not owned:
        raise AssertionError("free arithmetic result did not own its native storage")
    return expected_type, payload, int(owned)


def main():
    """Run one selected operation family and bounded exact-signature chunk."""
    family_index, start, end = map(int, sys.argv[1:4])
    family = FAMILIES[family_index]
    signatures = inventory_signatures(family)
    if not 0 <= start < end <= len(signatures):
        raise ValueError("operator matrix chunk is outside the selected signature list")
    sources = probe.Sources()
    native_function = getattr(dt, family.removeprefix("sc_dt::"))
    py_binary = PUBLIC_OPERATIONS[family_index]
    py_unary = UNARY_OPERATIONS[family_index]
    retained = {}
    operands = None
    selected = None
    public = None
    exact = None

    for index in range(start, end):
        signature = signatures[index]
        result_type = signature[: signature.index("(")].strip().removeprefix("const ")
        params = split_arguments(signature)
        try:
            operands = tuple(source_value(sources, spelling) for spelling in params)
            exact = native_function.__overload__(", ".join(params))
            selected = exact(*operands)
            exact_state = render(selected, result_type)
        except Exception as error:
            detail = str(error).replace("\t", " ").replace("\n", " ")
            print(f"MATRIX_ERROR\t{family_index}\t{index}\t{type(error).__name__}:{detail}\t{signature}", flush=True)
            continue
        print(f"MATRIX\t{family_index}\t{index}\t{exact_state[0]}|{exact_state[1]}", flush=True)
        try:
            if len(params) == 1:
                public = py_unary(operands[0])
            else:
                public = py_binary(*operands)
            public_state = render(public, result_type)
        except Exception as error:
            detail = str(error).replace("\t", " ").replace("\n", " ")
            print(f"PUBLIC_ERROR\t{family_index}\t{index}\t{type(error).__name__}:{detail}\t{signature}", flush=True)
        else:
            print(f"PUBLIC\t{family_index}\t{index}\t{public_state[0]}|{public_state[1]}", flush=True)
            print(f"OWNED\t{family_index}\t{index}\t{public_state[2]}", flush=True)
            if result_type not in retained:
                retained[result_type] = (index, public, public_state)

    del operands, selected, public, exact, sources
    gc.collect()
    for result_type, (index, value, expected_state) in retained.items():
        assert render(value, result_type) == expected_state, "operator result depended on destroyed source storage"
        print(f"LIFETIME\t{family_index}\t{index}\t{result_type}\t{expected_state[1]}", flush=True)
    print(f"SC_DT_ARITHMETIC_OPERATORS_PUBLIC_OK {family_index} {start} {end}")


if __name__ == "__main__":
    main()
