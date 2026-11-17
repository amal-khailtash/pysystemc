"""Compare Python bool construction and assignment with direct C++ calls."""

from pathlib import Path

from amal.eda.systemc import sc_dt as dt
import cppyy


PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "integer_bool_conversion_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.integer_bool_conversion_probe

CASES = (
    ("sc_int", 1, True),
    ("sc_int", 37, True),
    ("sc_int", 64, True),
    ("sc_uint", 1, False),
    ("sc_uint", 38, False),
    ("sc_uint", 64, False),
    ("sc_bigint", 1, True),
    ("sc_bigint", 137, True),
    ("sc_biguint", 1, False),
    ("sc_biguint", 257, False),
)
CONSTRUCTOR_FORMS = ("positional", "a", "v")


def _expected(value, width, signed):
    """Apply SystemC fixed-width wrapping to one Python bool value."""
    wrapped = int(value) & ((1 << width) - 1)
    if signed and wrapped >= (1 << (width - 1)):
        wrapped -= 1 << width
    return wrapped


def _render(value):
    """Render a Python-visible SystemC integer without narrowing its width."""
    spelling = value.to_string(dt.SC_DEC, False)
    return spelling.decode() if isinstance(spelling, bytes) else str(spelling)


def _case_type(family, width):
    """Return the requested arbitrary-width SystemC integer specialization."""
    return getattr(dt, family)[width]


def _construct(target_type, form, value):
    """Call a positional or supported single-value keyword constructor."""
    if form == "positional":
        return target_type(value)
    return target_type(**{form: value})


def exercise_case(family, width, signed):
    """Check bool constructors and assignment against direct C++ references.

    Args:
        family: SystemC integer template family name.
        width: Native template width to instantiate.
        signed: Whether the target interprets its highest bit as a sign bit.
    Returns:
        Deterministic output rows for the pytest parent to audit.
    Raises:
        AssertionError: Python behavior, storage ownership, or native parity differs.
    """
    target_type = _case_type(family, width)
    reference = probe.Reference[target_type]
    rows = []
    for bool_value in (False, True):
        expected = _expected(bool_value, width, signed)
        python_targets = [_construct(target_type, form, bool_value) for form in CONSTRUCTOR_FORMS]
        native_target = reference.construct(bool_value)
        python_values = [_render(target) for target in python_targets]
        native_value = str(reference.render(native_target))
        assert python_values == [str(expected)] * len(CONSTRUCTOR_FORMS), (
            family,
            width,
            bool_value,
            python_values,
            expected,
        )
        assert native_value == str(expected), (family, width, bool_value, native_value, expected)
        addresses = [int(cppyy.addressof(target)) for target in python_targets]
        native_address = int(cppyy.addressof(native_target))
        assert len(set(addresses + [native_address])) == len(CONSTRUCTOR_FORMS) + 1
        assert all(bool(target.__python_owns__) for target in python_targets)
        for form, _target, value in zip(CONSTRUCTOR_FORMS, python_targets, python_values, strict=True):
            rows.append(
                (
                    "construct",
                    family,
                    width,
                    int(bool_value),
                    form,
                    value,
                    native_value,
                    expected,
                    1,
                    1,
                )
            )

        owner = target_type(not bool_value)
        native_owner = reference.construct(not bool_value)
        owner_address = int(cppyy.addressof(owner))
        native_owner_address = int(cppyy.addressof(native_owner))
        result = owner.__assign__(bool_value)
        native_result = reference.assign(native_owner, bool_value)
        assigned_value = _render(owner)
        native_assigned_value = str(reference.render(native_owner))
        assert assigned_value == native_assigned_value == str(expected), (
            family,
            width,
            bool_value,
            assigned_value,
            native_assigned_value,
            expected,
        )
        assert int(cppyy.addressof(result)) == owner_address
        assert result is owner
        assert int(cppyy.addressof(native_result)) == native_owner_address
        rows.append(
            (
                "assign",
                family,
                width,
                int(bool_value),
                assigned_value,
                native_assigned_value,
                expected,
                1,
            )
        )
    return rows


def main():
    """Run and emit the complete bool conversion matrix."""
    rows = [row for family, width, signed in CASES for row in exercise_case(family, width, signed)]
    for row in rows:
        print("IBC\t" + "\t".join(map(str, row)))
    print(f"INTEGER_BOOL_CONVERSION_OK\t{len(rows)}")


if __name__ == "__main__":
    main()
