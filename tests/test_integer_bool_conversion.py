"""Verify bool construction and assignment across native integer widths."""

from .support import run_example


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


def expected_value(value, width, signed):
    """Return the fixed-width signed or unsigned interpretation of a bool."""
    wrapped = int(value) & ((1 << width) - 1)
    if signed and wrapped >= (1 << (width - 1)):
        wrapped -= 1 << width
    return wrapped


def test_integer_bool_construction_and_assignment_match_cpp(tmp_path):
    """Check all bool APIs against separate native C++ constructor/assign calls.

    Args:
        tmp_path: Isolated working directory for this fresh Python process.
    Raises:
        AssertionError: The model process, output matrix, ownership, or parity fails.
        subprocess.TimeoutExpired: The model exceeds its sixty-second bound.
    """
    output = run_example(tmp_path, "tests/models/integer_bool_conversion.py")
    records = [line.split("\t") for line in output.splitlines() if line.startswith("IBC\t")]
    expected_constructor_keys = {
        (family, width, value, form)
        for family, width, _signed in CASES
        for value in (0, 1)
        for form in CONSTRUCTOR_FORMS
    }
    expected_assignment_keys = {(family, width, value) for family, width, _signed in CASES for value in (0, 1)}
    constructor_records = {}
    assignment_records = {}
    for fields in records:
        if fields[1] == "construct":
            assert len(fields) == 11, fields
            key = (fields[2], int(fields[3]), int(fields[4]), fields[5])
            assert key not in constructor_records, key
            constructor_records[key] = fields
        elif fields[1] == "assign":
            assert len(fields) == 9, fields
            key = (fields[2], int(fields[3]), int(fields[4]))
            assert key not in assignment_records, key
            assignment_records[key] = fields
        else:
            raise AssertionError(f"unexpected bool conversion record: {fields!r}")

    assert set(constructor_records) == expected_constructor_keys
    assert set(assignment_records) == expected_assignment_keys
    signed_by_case = {(family, width): signed for family, width, signed in CASES}
    for (family, width, value, _form), fields in constructor_records.items():
        expected = str(expected_value(value, width, signed_by_case[family, width]))
        assert fields[6:9] == [expected, expected, expected], fields
        assert fields[9:11] == ["1", "1"], fields
    for (family, width, value), fields in assignment_records.items():
        expected = str(expected_value(value, width, signed_by_case[family, width]))
        assert fields[5:8] == [expected, expected, expected], fields
        assert fields[8] == "1", fields

    expected_count = len(expected_constructor_keys) + len(expected_assignment_keys)
    assert f"INTEGER_BOOL_CONVERSION_OK\t{expected_count}" in output
    assert len(records) == expected_count == 80
