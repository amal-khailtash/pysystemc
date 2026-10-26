"""Validate public integer-template constructors and assignments against C++."""

import csv
from collections import Counter
import subprocess
import sys

from .support import ROOT, run_example


EXPECTED_GROUPS = {
    "sc_dt::sc_bigint": 48,
    "sc_dt::sc_biguint": 48,
    "sc_dt::sc_int": 59,
    "sc_dt::sc_uint": 59,
}
EXPECTED_KIND_COUNTS = {
    "sc_bigint": {
        "ClassTemplateDecl": 1,
        "CXXRecordDecl": 1,
        "CXXConstructorDecl": 21,
        "CXXDestructorDecl": 1,
        "CXXMethodDecl": 22,
        "FunctionTemplateDecl": 2,
    },
    "sc_biguint": {
        "ClassTemplateDecl": 1,
        "CXXRecordDecl": 1,
        "CXXConstructorDecl": 21,
        "CXXDestructorDecl": 1,
        "CXXMethodDecl": 22,
        "FunctionTemplateDecl": 2,
    },
    "sc_int": {
        "ClassTemplateDecl": 1,
        "CXXRecordDecl": 1,
        "CXXConstructorDecl": 21,
        "CXXMethodDecl": 34,
        "FunctionTemplateDecl": 2,
    },
    "sc_uint": {
        "ClassTemplateDecl": 1,
        "CXXRecordDecl": 1,
        "CXXConstructorDecl": 21,
        "CXXMethodDecl": 34,
        "FunctionTemplateDecl": 2,
    },
}

BIG_CONSTRUCTOR_PARAMS = {
    "",
    "const sc_bigint<W> &",
    "const sc_dt::sc_signed &",
    "const sc_dt::sc_signed_subref &",
    "const sc_generic_base<T> &",
    "const sc_dt::sc_unsigned &",
    "const sc_dt::sc_unsigned_subref &",
    "const char *",
    "sc_dt::int64",
    "sc_dt::uint64",
    "long",
    "unsigned long",
    "int",
    "unsigned int",
    "double",
    "const sc_dt::sc_bv_base &",
    "const sc_dt::sc_lv_base &",
    "const sc_dt::sc_fxval &",
    "const sc_dt::sc_fxval_fast &",
    "const sc_dt::sc_fxnum &",
    "const sc_dt::sc_fxnum_fast &",
}
BIGUINT_CONSTRUCTOR_PARAMS = {value.replace("sc_bigint", "sc_biguint") for value in BIG_CONSTRUCTOR_PARAMS}
BIGINT_ASSIGNMENT_PARAMS = {
    "const sc_dt::sc_bv_base &",
    "sc_dt::int64",
    "const sc_dt::sc_signed &",
    "const sc_dt::sc_unsigned &",
    "const sc_bigint<W> &",
    "sc_dt::uint64",
    "const sc_dt::sc_unsigned_subref &",
    "const sc_dt::sc_uint_base &",
    "const sc_dt::sc_signed_subref &",
    "const sc_dt::sc_fxnum &",
    "const sc_dt::sc_fxval &",
    "const char *",
    "unsigned long",
    "const sc_generic_base<T> &",
    "long",
    "const sc_dt::sc_fxval_fast &",
    "double",
    "const sc_dt::sc_int_base &",
    "unsigned int",
    "const sc_dt::sc_lv_base &",
    "const sc_dt::sc_fxnum_fast &",
    "int",
}
BIGUINT_ASSIGNMENT_PARAMS = {value.replace("sc_bigint", "sc_biguint") for value in BIGINT_ASSIGNMENT_PARAMS}
FIXED_SIGNED_PARAMS = {
    "",
    "const sc_generic_base<T> &",
    "int",
    "const sc_dt::sc_bv_base &",
    "unsigned long",
    "sc_dt::int_type",
    "const sc_dt::sc_lv_base &",
    "const sc_dt::sc_int_subref_r &",
    "long",
    "const sc_dt::sc_signed &",
    "const sc_dt::sc_fxval &",
    "const sc_dt::sc_fxnum_fast &",
    "const sc_dt::sc_fxval_fast &",
    "const sc_dt::sc_int_base &",
    "const sc_int<W> &",
    "sc_dt::uint64",
    "const sc_dt::sc_fxnum &",
    "const char *",
    "const sc_dt::sc_unsigned &",
    "double",
    "unsigned int",
}
FIXED_UNSIGNED_PARAMS = {
    "",
    "sc_dt::uint_type",
    "const sc_dt::sc_fxnum_fast &",
    "const sc_dt::sc_bv_base &",
    "const sc_dt::sc_fxval &",
    "const sc_dt::sc_uint_subref_r &",
    "double",
    "const sc_dt::sc_lv_base &",
    "const sc_dt::sc_uint_base &",
    "const sc_dt::sc_unsigned &",
    "const sc_dt::sc_fxnum &",
    "const sc_generic_base<T> &",
    "unsigned int",
    "const char *",
    "unsigned long",
    "const sc_dt::sc_signed &",
    "sc_dt::int64",
    "const sc_uint<W> &",
    "int",
    "const sc_dt::sc_fxval_fast &",
    "long",
}
FIXED_SIGNED_ASSIGNMENT_PARAMS = {
    "const sc_dt::sc_fxnum &",
    "const sc_dt::sc_fxnum_fast &",
    "int",
    "const sc_dt::sc_fxval_fast &",
    "long",
    "const sc_dt::sc_fxval &",
    "const sc_int<W> &",
    "const sc_generic_base<T> &",
    "sc_dt::uint64",
    "sc_dt::int_type",
    "const sc_dt::sc_unsigned &",
    "const sc_dt::sc_lv_base &",
    "const char *",
    "const sc_dt::sc_int_subref_r &",
    "double",
    "const sc_dt::sc_signed &",
    "const sc_dt::sc_bv_base &",
    "const sc_dt::sc_int_base &",
    "unsigned int",
    "unsigned long",
}
FIXED_UNSIGNED_ASSIGNMENT_PARAMS = {
    "const sc_dt::sc_uint_subref_r &",
    "const sc_dt::sc_fxnum &",
    "const sc_dt::sc_uint_base &",
    "const sc_dt::sc_bv_base &",
    "const sc_dt::sc_lv_base &",
    "const sc_dt::sc_fxval &",
    "const char *",
    "const sc_generic_base<T> &",
    "long",
    "const sc_dt::sc_signed &",
    "const sc_dt::sc_fxnum_fast &",
    "sc_dt::int64",
    "const sc_uint<W> &",
    "double",
    "unsigned int",
    "unsigned long",
    "sc_dt::uint_type",
    "int",
    "const sc_dt::sc_unsigned &",
    "const sc_dt::sc_fxval_fast &",
}

BIG_CASES = (
    "signed",
    "signed_subref",
    "generic_concat",
    "unsigned",
    "unsigned_subref",
    "string",
    "int64",
    "uint64",
    "long",
    "ulong",
    "int",
    "uint",
    "double",
    "bv",
    "lv",
    "fxval",
    "fxval_fast",
    "fxnum",
    "fxnum_fast",
)
FIXED_COMMON_CASES = (
    "base",
    "subref",
    "generic_concat",
    "signed",
    "unsigned",
    "string",
    "long",
    "ulong",
    "int",
    "uint",
    "double",
    "bv",
    "lv",
    "fxval",
    "fxval_fast",
    "fxnum",
    "fxnum_fast",
)
FIXED_OPERATOR_RECORDS = (
    "plus_assign",
    "minus_assign",
    "multiply_assign",
    "divide_assign",
    "modulo_assign",
    "and_assign",
    "or_assign",
    "xor_assign",
    "left_shift_assign",
    "right_shift_assign",
    "prefix_increment",
    "postfix_increment_old",
    "postfix_increment_new",
    "prefix_decrement",
    "postfix_decrement_old",
    "postfix_decrement_new",
)


def inventory_rows():
    """Return the four template-family declarations in the public inventory.

    Returns:
        CSV declaration rows scoped to sc_bigint, sc_biguint, sc_int and sc_uint.
    """
    inventory = ROOT / "docs" / "_static" / "public-api-inventory.csv"
    with inventory.open(newline="", encoding="utf-8") as source:
        return [
            row
            for row in csv.DictReader(source)
            if any(row["name"] == family or row["name"].startswith(family + "::") for family in EXPECTED_GROUPS)
        ]


def signature_parameters(signature):
    """Extract the native parameter list from an inventory function signature.

    Args:
        signature: C++ return type and parameter-list spelling from the CSV.

    Returns:
        Exact parameter text inside the outermost pair of parentheses.
    """
    return signature[signature.find("(") + 1 : signature.rfind(")")]


def assert_inventory_signature_audit(rows):
    """Check all inventoried template rows and source-level overload sets.

    Args:
        rows: Declaration rows returned by ``inventory_rows``.
    """
    by_family = {
        family: [row for row in rows if row["name"] == family or row["name"].startswith(family + "::")]
        for family in EXPECTED_GROUPS
    }
    assert {family: len(family_rows) for family, family_rows in by_family.items()} == EXPECTED_GROUPS
    assert len(rows) == 214

    source_files = {
        "sc_bigint": "sc_bigint.h",
        "sc_biguint": "sc_biguint.h",
        "sc_int": "sc_int.h",
        "sc_uint": "sc_uint.h",
    }
    for qualified, family_rows in by_family.items():
        family = qualified.split("::")[-1]
        assert Counter(row["kind"] for row in family_rows) == EXPECTED_KIND_COUNTS[family]
        assert all(source_files[family] in row["source"] for row in family_rows)
        assert all(row["line"].isdigit() and row["signature"] is not None for row in family_rows)
        constructors = {
            signature_parameters(row["signature"]) for row in family_rows if row["kind"] == "CXXConstructorDecl"
        }
        assignments = {
            signature_parameters(row["signature"])
            for row in family_rows
            if row["kind"] == "CXXMethodDecl" and row["name"].endswith("operator=")
        }
        expected_constructors = {
            "sc_bigint": BIG_CONSTRUCTOR_PARAMS,
            "sc_biguint": BIGUINT_CONSTRUCTOR_PARAMS,
            "sc_int": FIXED_SIGNED_PARAMS,
            "sc_uint": FIXED_UNSIGNED_PARAMS,
        }[family]
        expected_assignments = {
            "sc_bigint": BIGINT_ASSIGNMENT_PARAMS,
            "sc_biguint": BIGUINT_ASSIGNMENT_PARAMS,
            "sc_int": FIXED_SIGNED_ASSIGNMENT_PARAMS,
            "sc_uint": FIXED_UNSIGNED_ASSIGNMENT_PARAMS,
        }[family]
        assert constructors == expected_constructors
        assert assignments == expected_assignments


def expected_records():
    """Build exact operation counts for every tested specialization.

    Returns:
        Counter keyed by (family, width, operation) for all native parity rows.
    """
    expected = Counter()
    copy_operations = (
        "copy_ctor",
        "copy_assign",
        "copy_assignment_self",
        "copy_independent",
        "copy_lifetime",
    )
    gate_operations = (
        "python_int_gate_expected",
        "python_int_gate_ctor",
        "python_int_gate_assign_expected",
        "python_int_gate_assign",
    )
    for family in ("sc_dt::sc_bigint", "sc_dt::sc_biguint"):
        for width in (48, 137, 191, 257):
            if width == 48:
                for operation in ("default_ctor", *copy_operations, *gate_operations):
                    expected[family, width, operation] += 1
                for label in BIG_CASES:
                    for operation in (f"ctor_{label}", f"assign_{label}", f"assign_{label}_self"):
                        expected[family, width, operation] += 1
                for label in ("int_base", "uint_base"):
                    for operation in (f"assign_{label}", f"assign_{label}_self"):
                        expected[family, width, operation] += 1
            else:
                for operation in (
                    "width_probe_ctor",
                    "width_probe_assign",
                    "width_probe_assign_self",
                    "width_probe_copy",
                ):
                    expected[family, width, operation] += 1
    for family in ("sc_dt::sc_int", "sc_dt::sc_uint"):
        for width in (19, 59, 63, 64):
            if width == 59:
                for operation in ("default_ctor", *copy_operations, *gate_operations, *FIXED_OPERATOR_RECORDS):
                    expected[family, width, operation] += 1
                typed_scalar_labels = ("int_type", "uint64") if family.endswith("sc_int") else ("uint_type", "int64")
                for label in (*FIXED_COMMON_CASES, *typed_scalar_labels):
                    for operation in (f"ctor_{label}", f"assign_{label}", f"assign_{label}_self"):
                        expected[family, width, operation] += 1
            else:
                for operation in (
                    "width_probe_ctor",
                    "width_probe_assign",
                    "width_probe_assign_self",
                    "width_probe_copy",
                ):
                    expected[family, width, operation] += 1
    for family, width in (
        ("sc_dt::sc_bigint", 48),
        ("sc_dt::sc_biguint", 48),
        ("sc_dt::sc_int", 59),
        ("sc_dt::sc_uint", 59),
    ):
        for operation in ("invalid_string_ctor", "invalid_string_assign"):
            expected[family, width, operation] += 1
    for operation in (
        "python_large_int_ctor_expected",
        "python_large_int_ctor",
        "python_large_int_assign_expected",
        "python_large_int_assign",
    ):
        expected["sc_dt::sc_biguint", 16384, operation] += 1
    return expected


def test_sc_dt_integer_templates_match_native_reference(tmp_path):
    """Compare each constructor, assignment, fixed-point and operator overload.

    The selected public families contain 214 inventory rows. Arbitrary-width
    specializations are exercised at 48, 137, 191 and 257 bits; bounded
    specializations use 19, 59, 63 and 64 bits. Native C++ and Python each run
    in a separate fresh subprocess because SystemC retains process state.
    """
    rows = inventory_rows()
    assert_inventory_signature_audit(rows)

    python_output = run_example(tmp_path, "tests/models/sc_dt_integer_templates_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples" / "run_sysc_cpp.py"),
            str(ROOT / "tests" / "native" / "sc_dt_integer_templates_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("ITM\t")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("ITM\t")]
    if python_records != native_records:
        for index, (python_record, native_record) in enumerate(zip(python_records, native_records)):
            if python_record != native_record:
                raise AssertionError(f"record {index} differs:\nPython: {python_record}\nNative: {native_record}")
        raise AssertionError(f"record counts differ: Python={len(python_records)} native={len(native_records)}")

    observed = Counter((parts[1], int(parts[2]), parts[3]) for line in python_records if (parts := line.split("\t")))
    assert observed == expected_records()
    assert len(python_records) == sum(expected_records().values())
    assert all(
        tuple(line.split("\t")[1:])[-1] == "1"
        for line in python_records
        if line.split("\t")[3].startswith("invalid_string_")
    )
    huge_value = next(line.split("\t")[-1] for line in python_records if "\tpython_large_int_ctor\t" in line)
    assert len(huge_value) > 4300
    assert "SC_DT_INTEGER_TEMPLATES_PUBLIC_OK" in python_output
