"""Exercise exact fixed-value constructors, native arithmetic and observers."""

import csv
import gc
import operator
from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

ROOT = Path(__file__).resolve().parents[2]
cppyy.include(str(ROOT / "tests/native/sc_dt_fxvalues_probe.h"))
probe = cppyy.gbl.sc_dt_fxvalues_probe


def inventory_rows():
    """Return every inventoried declaration for both public fixed values."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        return list(csv.DictReader(source))


def arguments(signature):
    """Split a signature into its native parameter types."""
    return [part.strip() for part in signature[signature.index("(") + 1 : signature.rindex(")")].split(", ")]


def observe(value):
    """Render decimal value, conversions, and native state predicates."""
    return "|".join(
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


def emit(family, label, value):
    """Print one deterministic record for native parity comparison."""
    print(f"{family}\t{label}\t{value}")


def source_values(sources):
    """Return operands through exact native source classes and scalar factories."""
    view, scalar = probe.SourceView, probe.Scalars
    return {
        "const sc_dt::sc_fxnum &": view.number(sources),
        "const sc_dt::sc_fxnum_fast &": view.number_fast(sources),
        "const sc_dt::sc_fxval &": view.value(sources),
        "const sc_dt::sc_fxval_fast &": view.value_fast(sources),
        "const sc_dt::sc_int_base &": view.sint(sources),
        "const sc_dt::sc_uint_base &": view.uint(sources),
        "const sc_dt::sc_signed &": view.signed_value(sources),
        "const sc_dt::sc_unsigned &": view.unsigned_value(sources),
        "int": scalar.integer(),
        "unsigned int": scalar.unsigned_integer(),
        "long": scalar.long_integer(),
        "unsigned long": scalar.unsigned_long_integer(),
        "float": scalar.real_float(),
        "double": scalar.real_double(),
        "sc_dt::int64": dt.int64(-1234567890123),
        "sc_dt::uint64": dt.uint64(1234567890123),
        "const char *": scalar.text(),
    }


def cpp_type(parameter):
    """Return the exact template argument type corresponding to one parameter."""
    if parameter == "const char *":
        return "std::string"
    if parameter.startswith("const ") and parameter.endswith(" &"):
        return parameter[len("const ") : -len(" &")]
    return parameter


def construct(owner_type, signature, values):
    """Construct an uninitialized wrapper through its exact overload selector."""
    params = arguments(signature)
    owner = owner_type.__new__(owner_type)
    if len(params) == 1:
        owner_type.__init__.__overload__(", ".join(params))(owner, cppyy.nullptr)
    else:
        owner_type.__init__.__overload__(", ".join(params))(owner, values[params[0]], cppyy.nullptr)
    return owner


rows = inventory_rows()
sources = probe.Sources()
values = source_values(sources)
assert type(values["sc_dt::int64"]).__cpp_name__ == "sc_dt::int64"
assert type(values["sc_dt::uint64"]).__cpp_name__ == "sc_dt::uint64"

# Exercise each distinct constructor selector; the frozen inventory also checks duplicate declarations.
for type_name, value_type in (("sc_dt::sc_fxval", dt.sc_fxval), ("sc_dt::sc_fxval_fast", dt.sc_fxval_fast)):
    constructors = [
        row
        for row in rows
        if row["name"] == f"{type_name}::{type_name.split('::')[-1]}" and row["kind"] == "CXXConstructorDecl"
    ]
    seen = set()
    for row in constructors:
        params = arguments(row["signature"])
        source_type = params[0] if len(params) > 1 else None
        if source_type in seen:
            continue
        seen.add(source_type)
        label = f"{type_name}|{source_type or 'observer'}"
        emit("FXCTOR", label, observe(construct(value_type, row["signature"], values)))
    assert len(seen) == 18

# Exact assignment members cover all declared source types.
for type_name, value_type in (("sc_dt::sc_fxval", dt.sc_fxval), ("sc_dt::sc_fxval_fast", dt.sc_fxval_fast)):
    selected = [row for row in rows if row["name"] == f"{type_name}::operator=" and row["kind"] == "CXXMethodDecl"]
    unique = {arguments(row["signature"])[0] for row in selected}
    assert len(unique) == 17
    for param in unique:
        target = value_type(0)
        target.__assign__.__overload__(param)(values[param])
        emit("FXASSIGN", f"{type_name}|{param}", observe(target))

# Typed compound helpers reproduce each source signature and preserve target identity.
compound_methods = {
    "operator+=": ("add", operator.iadd, 0),
    "operator-=": ("subtract", operator.isub, 1),
    "operator*=": ("multiply", operator.imul, 2),
    "operator/=": ("divide", operator.itruediv, 3),
    "operator<<=": ("left_shift", operator.ilshift, 4),
    "operator>>=": ("right_shift", operator.irshift, 5),
}
for type_name, value_type in (("sc_dt::sc_fxval", dt.sc_fxval), ("sc_dt::sc_fxval_fast", dt.sc_fxval_fast)):
    target_cpp = type_name
    for method_name, (adapter_name, python_mutation, operation) in compound_methods.items():
        selected = [
            row for row in rows if row["name"] == f"{type_name}::{method_name}" and row["kind"] == "CXXMethodDecl"
        ]
        for param in {arguments(row["signature"])[0] for row in selected}:
            source = 2 if operation >= 4 else values[param]
            value = value_type(3.5)
            if operation >= 4:
                python_mutation(value, source)
            else:
                adapter = cppyy.gbl.pysystemc_datatypes.NumericMutation[value_type, cpp_type(param)]
                getattr(adapter, adapter_name)(value, source)
            emit("FXCOMPOUND", f"{target_cpp}::{method_name}|{param}", observe(value))

# Run every exact named add/subtract/multiply/divide overload for each result type.
for type_name, value_type in (("sc_dt::sc_fxval", dt.sc_fxval), ("sc_dt::sc_fxval_fast", dt.sc_fxval_fast)):
    selected = [
        row for row in rows if row["name"] == "sc_dt::add" and row["signature"].startswith(f"void ({type_name} &,")
    ]
    pairs = {}
    for row in selected:
        params = arguments(row["signature"])
        pairs[tuple(params[1:])] = params
    assert len(pairs) == 64
    for operands, params in pairs.items():
        left, right = values[operands[0]], values[operands[1]]
        output = value_type(0)
        results = []
        for operation in ("add", "sub", "mult", "div"):
            getattr(dt, operation).__overload__(", ".join(params))(output, left, right)
            results.append(str(output.to_string(dt.SC_DEC)))
        emit("FXBIN", f"{type_name} &,{operands[0]},{operands[1]}", "|".join(results))

# Exact native arithmetic adapter types cover all declared friend operand orderings.
for type_name in ("sc_dt::sc_fxval", "sc_dt::sc_fxval_fast"):
    selected = [
        row for row in rows if row["name"] == "sc_dt::operator+" and row["signature"].startswith(f"const {type_name} (")
    ]
    pairs = {}
    for row in selected:
        pair = tuple(arguments(row["signature"]))
        pairs[pair] = pair
    for left_type, right_type in pairs:
        left, right = values[left_type], values[right_type]
        adapter = cppyy.gbl.pysystemc_datatypes.NumericOperators[cpp_type(left_type), cpp_type(right_type)]
        result = [
            str(getattr(adapter, method)(left, right).to_string(dt.SC_DEC))
            for method in ("add", "subtract", "multiply", "divide")
        ]
        emit("FXOP", f"{type_name} &,{left_type},{right_type}", "|".join(result))

# Keep values independent of source destruction and compare normal/fast conversion precision.
source = dt.sc_fxval("7.125")
copy = dt.sc_fxval(source)
del source
gc.collect()
emit("FXLIFE", "copy_after_source_release", observe(copy))

for label, value, stream, dump in (
    ("value", dt.sc_fxval(3.5), probe.Formatting.stream_value, probe.Formatting.dump_value),
    ("fast", dt.sc_fxval_fast(3.5), probe.Formatting.stream_fast, probe.Formatting.dump_fast),
):
    emit("FXOBS", label, observe(value))
    emit("FXFMT", label, str(stream(value)).replace("\n", "~"))
    emit("FXDUMP", label, str(dump(value)).replace("\n", "~"))

precise_text = "1.23456789012345678901234567890123456789"
precise = dt.sc_fxval(precise_text)
fast_precise = dt.sc_fxval_fast(precise_text)
precision = "|".join(
    (
        str(precise.to_string(dt.SC_DEC)),
        str(fast_precise.to_string(dt.SC_DEC)),
        format(float(precise.to_double()), ".17g"),
        format(float(fast_precise.to_double()), ".17g"),
    )
)
assert str(precise.to_string(dt.SC_DEC)) != str(fast_precise.to_string(dt.SC_DEC))
emit("FXPREC", "normal_vs_fast", precision)

# Ordinary Python int/float expressions verify the production numeric wrappers.
for type_name, value_type in (("value", dt.sc_fxval), ("fast", dt.sc_fxval_fast)):
    for source_name, source in (("python_int", 2), ("python_float", 1.25), ("fixed_value", dt.sc_fxval_fast(0.75))):
        left = value_type(3.5)
        results = [
            str(expression(left, source).to_string(dt.SC_DEC))
            for expression in (operator.add, operator.sub, operator.mul, operator.truediv)
        ]
        emit("FXPY", f"{type_name}|{source_name}|arithmetic", "|".join(results))

print("SC_DT_FXVALUES_PUBLIC_OK")
