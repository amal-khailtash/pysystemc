"""Compare the public sc_bit and sc_logic families with native SystemC."""

import csv
from collections import Counter
import subprocess
import sys

from .support import ROOT, run_example

FAMILIES = {
    "sc_dt::sc_bit": Counter(
        {
            ("CXXRecordDecl", "sc_dt::sc_bit"): 2,
            ("CXXConstructorDecl", "sc_dt::sc_bit::sc_bit"): 11,
            ("CXXDestructorDecl", "sc_dt::sc_bit::~sc_bit"): 1,
            ("CXXMethodDecl", "sc_dt::sc_bit::b_not"): 1,
            ("CXXMethodDecl", "sc_dt::sc_bit::operator!"): 1,
            ("CXXMethodDecl", "sc_dt::sc_bit::operator&="): 4,
            ("CXXMethodDecl", "sc_dt::sc_bit::operator="): 9,
            ("CXXMethodDecl", "sc_dt::sc_bit::operator^="): 4,
            ("CXXMethodDecl", "sc_dt::sc_bit::operator|="): 4,
            ("CXXMethodDecl", "sc_dt::sc_bit::print"): 1,
            ("CXXMethodDecl", "sc_dt::sc_bit::scan"): 1,
            ("CXXMethodDecl", "sc_dt::sc_bit::to_bool"): 1,
            ("CXXMethodDecl", "sc_dt::sc_bit::to_char"): 1,
            ("CXXConversionDecl", "sc_dt::sc_bit::operator bool"): 1,
        }
    ),
    "sc_dt::sc_logic": Counter(
        {
            ("CXXRecordDecl", "sc_dt::sc_logic"): 5,
            ("CXXConstructorDecl", "sc_dt::sc_logic::sc_logic"): 7,
            ("CXXDestructorDecl", "sc_dt::sc_logic::~sc_logic"): 1,
            ("CXXMethodDecl", "sc_dt::sc_logic::b_not"): 1,
            ("CXXMethodDecl", "sc_dt::sc_logic::is_01"): 1,
            ("CXXMethodDecl", "sc_dt::sc_logic::operator&="): 6,
            ("CXXMethodDecl", "sc_dt::sc_logic::operator="): 6,
            ("CXXMethodDecl", "sc_dt::sc_logic::operator^="): 6,
            ("CXXMethodDecl", "sc_dt::sc_logic::operator|="): 6,
            ("CXXMethodDecl", "sc_dt::sc_logic::operator~"): 1,
            ("CXXMethodDecl", "sc_dt::sc_logic::print"): 1,
            ("CXXMethodDecl", "sc_dt::sc_logic::scan"): 1,
            ("CXXMethodDecl", "sc_dt::sc_logic::to_bool"): 1,
            ("CXXMethodDecl", "sc_dt::sc_logic::to_char"): 1,
            ("CXXMethodDecl", "sc_dt::sc_logic::value"): 1,
            ("VarDecl", "sc_dt::sc_logic::and_table"): 1,
            ("VarDecl", "sc_dt::sc_logic::char_to_logic"): 1,
            ("VarDecl", "sc_dt::sc_logic::logic_to_char"): 1,
            ("VarDecl", "sc_dt::sc_logic::not_table"): 1,
            ("VarDecl", "sc_dt::sc_logic::or_table"): 1,
            ("VarDecl", "sc_dt::sc_logic::xor_table"): 1,
        }
    ),
}


def inventory_matrix():
    """Read the precise class and member rows assigned to this audit.

    Returns:
        A counter keyed by public class family and (declaration kind, name).
    """
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as inventory_file:
        inventory = csv.DictReader(inventory_file)
        rows = Counter()
        for row in inventory:
            parts = row["name"].split("::")
            family = "::".join(parts[:2])
            if family in FAMILIES:
                rows[(family, row["kind"], row["name"])] += 1
    return rows


def test_sc_dt_logic_public(tmp_path):
    """Match all 93 class/member declarations to a native behavior matrix."""
    inventory = inventory_matrix()
    for family, expected in FAMILIES.items():
        actual = Counter(
            {(kind, name): count for (row_family, kind, name), count in inventory.items() if row_family == family}
        )
        assert actual == expected
    assert sum(sum(expected.values()) for expected in FAMILIES.values()) == 93

    python_output = run_example(tmp_path, "tests/models/sc_dt_logic_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_logic_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefix = "DT_LOGIC\t"
    actual_records = [line for line in python_output.splitlines() if line.startswith(prefix)]
    native_records = [line for line in native.stdout.splitlines() if line.startswith(prefix)]
    assert actual_records == native_records
    actual_rows = [line.split("\t") for line in actual_records]
    category_counts = Counter(row[1] for row in actual_rows)
    assert category_counts == Counter(
        {
            "bit_ctor": 22,
            "bit_assign": 19,
            "bit_compound": 48,
            "bit_convert": 2,
            "bit_b_not": 2,
            "bit_scan": 2,
            "bit_invalid_operation": 8,
            "logic_ctor": 21,
            "logic_assign": 24,
            "logic_compound": 288,
            "logic_convert": 4,
            "logic_b_not": 4,
            "logic_scan": 4,
            "logic_table": 6,
            "logic_invalid": 2,
            "lifetime": 1,
        }
    )

    tables = {row[2]: row[3] for row in actual_rows if row[1] == "logic_table"}
    expected_tables = {
        "and_table": "0,0,0,0,0,1,3,3,0,3,3,3,0,3,3,3",
        "or_table": "0,1,3,3,1,1,1,1,3,1,3,3,3,1,3,3",
        "xor_table": "0,1,3,3,1,0,3,3,3,3,3,3,3,3,3,3",
        "not_table": "1,0,3,3",
        "logic_to_char": "01ZX",
    }
    assert {name: tables[name] for name in expected_tables} == expected_tables
    expected_char_table = [3] * 128
    expected_char_table[:3] = [0, 1, 2]
    expected_char_table[ord("0")] = 0
    expected_char_table[ord("1")] = 1
    expected_char_table[ord("Z")] = expected_char_table[ord("z")] = 2
    assert [int(value) for value in tables["char_to_logic"].split(",")] == expected_char_table

    state_order = "01ZX"
    logic_pairs = {
        (row[2], row[3], row[5]): row[6] for row in actual_rows if row[1] == "logic_compound" and row[4] == "logic"
    }
    expected = {
        "and": ("0000", "01XX", "0XXX", "0XXX"),
        "or": ("01XX", "1111", "X1XX", "X1XX"),
        "xor": ("01XX", "10XX", "XXXX", "XXXX"),
    }
    for operation, rows in expected.items():
        for left_index, left in enumerate(state_order):
            for right_index, right in enumerate(state_order):
                assert logic_pairs[(operation, left, right)] == rows[left_index][right_index]


def test_sc_bit_invalid_inputs_are_native_errors(tmp_path):
    """Keep sc_bit's fatal invalid-value cases isolated from valid matrices."""
    for mode, marker in (("bit-error-int", "sc_bit( 2 )"), ("bit-error-char", "sc_bit( 'Q' )")):
        python_error = subprocess.run(
            [sys.executable, str(ROOT / "tests/models/sc_dt_logic_public.py"), mode],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert python_error.returncode != 0
        assert marker in python_error.stdout + python_error.stderr

        native_error = subprocess.run(
            [
                sys.executable,
                str(ROOT / "examples/run_sysc_cpp.py"),
                str(ROOT / "tests/native/sc_dt_logic_public.cpp"),
                "--",
                mode,
            ],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert native_error.returncode != 0
        assert marker in native_error.stdout + native_error.stderr
