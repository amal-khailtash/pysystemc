"""Compare all exact public named fixed-point arithmetic calls with native C++."""

import csv
import gc
import sys
from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy

ROOT = Path(__file__).resolve().parents[2]
OPERATIONS = ("add", "sub", "mult", "div")
OUTPUTS = ("sc_dt::sc_fxval", "sc_dt::sc_fxval_fast", "sc_dt::sc_fxnum", "sc_dt::sc_fxnum_fast")
PROBE_HEADER = ROOT / "tests/native/sc_dt_named_arithmetic_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.sc_dt_named_arithmetic_probe


def inventory_rows():
    """Load the frozen declarations for all four native named functions."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        return [
            row for row in csv.DictReader(source) if row["name"] in {f"sc_dt::{operation}" for operation in OPERATIONS}
        ]


def arguments(signature):
    """Split a native function signature into its exact argument spellings."""
    return tuple(part.strip() for part in signature[signature.index("(") + 1 : signature.rindex(")")].split(", "))


def normalized(spelling):
    """Match C++ macro-stringized type spellings with CSV type names."""
    return " ".join(spelling.split())


def signature_matrix(rows, output):
    """Return the reusable exact output/operand signature map for one output base."""
    result = {operation: {} for operation in OPERATIONS}
    for row in rows:
        operation = row["name"].split("::")[-1]
        params = arguments(row["signature"])
        if normalized(params[0]) == f"{output} &":
            result[operation][tuple(normalized(param) for param in params[1:])] = params
    return result


def source_values(side):
    """Own typed source values matching every scalar and class parameter."""
    if side not in (0, 1):
        raise ValueError("source side must be zero or one")
    scalar = {
        "int": (-7, 3),
        "unsigned int": (probe.UnsignedInt(0xA5A55A5A), probe.UnsignedInt(37)),
        "long": (probe.Long(-0x123456789ABCDEF), probe.Long(17)),
        "unsigned long": (probe.UnsignedLong(0x123456789ABCDEF), probe.UnsignedLong(19)),
        "float": (probe.Float(1.375), probe.Float(3.25)),
        "double": (2.625, 5.5),
        "const char *": ("7.8125", "3.25"),
        "sc_dt::int64": (dt.int64(-0x123456789ABCDEF), dt.int64(11)),
        "sc_dt::uint64": (dt.uint64(0xFEDCBA9876543210), dt.uint64(13)),
    }
    value = dt.sc_fxval("1.23456789012345678901234567890123456789")
    value_fast = dt.sc_fxval_fast("2.7182818284590452353602874713527")
    number = dt.sc_fixed[137, 100]("9007199254740992.125")
    number_fast = dt.sc_fixed_fast[137, 100]("9007199254740992.125")
    sint = dt.sc_int[59]("-9007199254740993")
    uint = dt.sc_uint[59]("9007199254740995")
    signed = dt.sc_bigint[137]("-9007199254740993")
    unsigned = dt.sc_biguint[191]("9007199254740995")
    native = {
        "const sc_dt::sc_fxnum &": number,
        "const sc_dt::sc_fxnum_fast &": number_fast,
        "const sc_dt::sc_fxval &": value,
        "const sc_dt::sc_fxval_fast &": value_fast,
        "const sc_dt::sc_int_base &": sint,
        "const sc_dt::sc_uint_base &": uint,
        "const sc_dt::sc_signed &": signed,
        "const sc_dt::sc_unsigned &": unsigned,
    }
    return {**{name: pair[side] for name, pair in scalar.items()}, **native}


def new_target(output, observer):
    """Create an owning output with a 191-bit native numeric destination."""
    if output == "sc_dt::sc_fxval":
        return dt.sc_fxval("0.125")
    if output == "sc_dt::sc_fxval_fast":
        return dt.sc_fxval_fast("0.125")
    if output == "sc_dt::sc_fxnum":
        return dt.sc_fixed[191, 140]("0.125", observer)
    if output == "sc_dt::sc_fxnum_fast":
        return dt.sc_fixed_fast[191, 140]("0.125", observer)
    raise ValueError(f"unknown output base {output}")


def render(value, output):
    """Render output value, full-precision decimal and family-specific state."""
    if output in ("sc_dt::sc_fxnum", "sc_dt::sc_fxnum_fast"):
        fields = (
            format(float(value.to_double()), ".17g"),
            str(value.to_string(dt.SC_DEC)),
            str(int(value.to_int())),
            str(int(value.to_uint())),
            str(int(value.wl())),
            str(int(value.iwl())),
            str(int(value.q_mode())),
            str(int(value.o_mode())),
            str(int(value.n_bits())),
            str(int(value.is_neg())),
            str(int(value.is_zero())),
            str(int(value.quantization_flag())),
            str(int(value.overflow_flag())),
            str(int(value.cast_switch() == dt.SC_ON)),
        )
    else:
        fields = (
            format(float(value.to_double()), ".17g"),
            str(value.to_string(dt.SC_DEC)),
            str(int(value.is_neg())),
            str(int(value.is_zero())),
            str(int(value.is_nan())),
            str(int(value.is_inf())),
            str(int(value.is_normal())),
            str(int(value.rounding_flag())),
        )
    return "|".join(fields)


def observer_matches(output, target, observer):
    """Check exact borrowed observer identity for numeric output families."""
    if output == "sc_dt::sc_fxnum":
        return bool(probe.observer_matches(target, observer))
    if output == "sc_dt::sc_fxnum_fast":
        return bool(probe.fast_observer_matches(target, observer))
    return False


def observer_capability(output):
    """Report observer support only for output bases that carry observers."""
    return bool(output in ("sc_dt::sc_fxnum", "sc_dt::sc_fxnum_fast") and probe.observers_enabled())


def emit(record):
    """Write a stable parity record consumed by the Python test module."""
    print("NAR\t" + "\t".join(map(str, record)))


def call_and_capture(output, operation, selector, args, observer):
    """Call a selected native arithmetic path and capture stable output state."""
    target = new_target(output, observer)
    address = cppyy.addressof(target)
    owned = bool(target.__python_owns__)
    result = selector(target, *args)
    assert result is None, "native output-parameter arithmetic must return None"
    state = render(target, output)
    same = cppyy.addressof(target) == address
    borrowed_observer = observer_matches(output, target, observer)
    assert owned and same, "named arithmetic replaced or borrowed the Python output owner"
    if output in ("sc_dt::sc_fxnum", "sc_dt::sc_fxnum_fast"):
        assert borrowed_observer, "named arithmetic changed the output's borrowed observer"
    record = (output, operation, state, int(same), int(borrowed_observer), int(observer_capability(output)))
    del target
    gc.collect()
    return record


def run_alias(output, operation, direction, observer, left, right, signature):
    """Exercise fast target identity as an operand through both native paths."""
    if output not in ("sc_dt::sc_fxval_fast", "sc_dt::sc_fxnum_fast"):
        raise ValueError("alias parity is defined only for fast fixed-point outputs")
    alias_types = arguments(signature)
    exact = getattr(dt, operation).__overload__(", ".join(alias_types))
    target = new_target(output, observer)
    address = cppyy.addressof(target)
    if direction == "left":
        operands = (target, right)
    else:
        operands = (left, target)
    getattr(dt, operation)(target, *operands)
    public_state = render(target, output)
    public_observer = observer_matches(output, target, observer)
    assert cppyy.addressof(target) == address and target.__python_owns__
    if output in ("sc_dt::sc_fxnum", "sc_dt::sc_fxnum_fast"):
        assert public_observer
    direct_target = new_target(output, observer)
    exact_operands = (direct_target, right) if direction == "left" else (left, direct_target)
    exact(direct_target, *exact_operands)
    direct_state = render(direct_target, output)
    assert direct_state == public_state, "public alias dispatch differs from its exact native selector"
    assert cppyy.addressof(direct_target) != address
    assert observer_matches(output, direct_target, observer) == public_observer
    record = (
        output,
        operation,
        direction,
        public_state,
        int(cppyy.addressof(target) == address),
        int(public_observer),
        int(observer_capability(output)),
    )
    del target, direct_target
    gc.collect()
    print("NARA\t" + "\t".join(map(str, record)))


def main():
    """Run the selected output-base half of the complete named function matrix."""
    output_index, mode = map(int, sys.argv[1:3])
    output = OUTPUTS[output_index]
    if mode not in (0, 1):
        raise ValueError("matrix mode must be zero or one")
    rows = inventory_rows()
    matrix = signature_matrix(rows, output)
    pairs = sorted(matrix["add"])
    midpoint = (len(pairs) + 1) // 2
    selected = pairs[:midpoint] if mode == 0 else pairs[midpoint:]
    observer = (
        probe.Observer()
        if output == "sc_dt::sc_fxnum"
        else (probe.FastObserver() if output == "sc_dt::sc_fxnum_fast" else None)
    )
    sources_left, sources_right = source_values(0), source_values(1)
    assert str(sources_left["const sc_dt::sc_fxval &"].to_string(dt.SC_DEC)) != str(
        sources_left["const sc_dt::sc_fxval_fast &"].to_string(dt.SC_DEC)
    ), "arbitrary and fast source precision collapsed"

    for pair in selected:
        left_name, right_name = pair
        left, right = sources_left[left_name], sources_right[right_name]
        for operation in OPERATIONS:
            params = matrix[operation][pair]
            signature = ", ".join(params)
            public = call_and_capture(output, operation, getattr(dt, operation), (left, right), observer)
            exact = getattr(dt, operation).__overload__(signature)
            direct = call_and_capture(output, operation, exact, (left, right), observer)
            assert public[2:] == direct[2:], (operation, pair, public, direct)
            emit((output, operation, left_name, right_name, *public[2:]))

    if mode == 0:
        observer_type = (
            probe.Observer
            if output == "sc_dt::sc_fxnum"
            else (probe.FastObserver if output == "sc_dt::sc_fxnum_fast" else None)
        )
        if output in ("sc_dt::sc_fxval", "sc_dt::sc_fxnum"):
            reason = (
                "SC_FXVAL_OUTPUT_MUST_NOT_ALIAS_INPUT"
                if output == "sc_dt::sc_fxval"
                else "DEFN_BIN_FNC_OUTPUT_MUST_NOT_ALIAS_INPUT"
            )
            print(f"NAR_ALIAS_UNSUPPORTED\t{output}\t{reason}")
        else:
            alias_pair = (f"const {output} &", f"const {output} &")
            for operation in OPERATIONS:
                params = matrix[operation].get(alias_pair)
                assert params is not None, f"missing exact target alias signature: {operation} {alias_pair}"
                base_source_left = sources_left[alias_pair[0]]
                base_source_right = sources_right[alias_pair[1]]
                run_alias(
                    output,
                    operation,
                    "left",
                    observer,
                    base_source_left,
                    base_source_right,
                    f"void ({output} &, {alias_pair[0]}, {alias_pair[1]})",
                )
                run_alias(
                    output,
                    operation,
                    "right",
                    observer,
                    base_source_left,
                    base_source_right,
                    f"void ({output} &, {alias_pair[0]}, {alias_pair[1]})",
                )
        if observer is not None:
            assert observer.__python_owns__
            destroyed = observer_type.destroyed
            gc.collect()
            assert observer_type.destroyed == destroyed, "output lifetime released its borrowed observer"
            if probe.observers_enabled():
                assert observer.writes > 0, "observer capability was enabled but no output write callback ran"
            del observer
            gc.collect()
            assert observer_type.destroyed == destroyed + 1, "Python owner did not release its native observer"
            observer_lifetime = 1
        else:
            observer_lifetime = 0
        print(
            "NAMED_ARITH_LIFETIME\t"
            + output
            + "\t"
            + str(observer_lifetime)
            + "\t"
            + str(int(observer_capability(output)))
        )
    print(f"SC_DT_NAMED_ARITHMETIC_PUBLIC_OK {output_index} {mode}")


if __name__ == "__main__":
    main()
