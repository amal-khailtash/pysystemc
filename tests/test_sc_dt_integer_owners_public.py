"""Audit the arbitrary-width signed and unsigned native integer owners."""

from collections import Counter
import csv
import hashlib
import subprocess
import sys

from .support import ROOT, run_example


INVENTORY_DIGESTS = {
    "sc_dt::sc_signed": "9cc8c03a2636d3a3166c86cb68cc6eb9c6b7a46f84ca9558d8fe2882056c4b80",
    "sc_dt::sc_unsigned": "3b3c3ec43095570ceaed3e022e4b7852595270118ae4eb2795507fe54096e593",
}
EXPECTED_KIND_COUNTS = {
    "CXXRecordDecl": 6,
    "CXXConstructorDecl": 10,
    "CXXMethodDecl": 181,
    "CXXConversionDecl": 0,
    "CXXDestructorDecl": 1,
    "FunctionTemplateDecl": 2,
}
PHASES = ("constructors", "compounds", "storage", "errors")
EXPECTED_CONSTRUCTOR_LABELS = {
    "width",
    "copy",
    "signed",
    "unsigned",
    "signed_range",
    "unsigned_range",
    "int_range",
    "uint_range",
    "bv",
    "lv",
    "generic_concat",
}
EXPECTED_ASSIGN_LABELS = {
    "signed",
    "unsigned",
    "signed_range",
    "unsigned_range",
    "sc_int_base",
    "sc_uint_base",
    "bv",
    "lv",
    "generic_concat",
    "string",
    "int",
    "unsigned_int",
    "long",
    "unsigned_long",
    "int64",
    "uint64",
    "double",
}
EXPECTED_COMPOUND_LABELS = {
    f"{operation}_{operand}"
    for operation in (
        "add",
        "subtract",
        "multiply",
        "divide",
        "remainder",
        "and",
        "or",
        "xor",
        "left_shift",
        "right_shift",
    )
    for operand in (
        "int",
        "unsigned_int",
        "long",
        "unsigned_long",
        "int64",
        "uint64",
        "sc_int_base",
        "sc_uint_base",
        "sc_signed",
        "sc_unsigned",
    )
}


def integer_owner_rows():
    """Return exact public class rows, excluding neighboring proxy classes."""
    path = ROOT / "docs" / "_static" / "public-api-inventory.csv"
    with path.open(newline="", encoding="utf-8") as source:
        all_rows = list(csv.DictReader(source))
    return {
        family: [row for row in all_rows if row["name"] == family or row["name"].startswith(family + "::")]
        for family in INVENTORY_DIGESTS
    }


def stable_digest(rows):
    """Hash stable declaration identity while ignoring mutable status fields."""
    canonical = "".join(
        "\t".join(row[field] for field in ("name", "kind", "signature", "source", "line")) + "\n" for row in rows
    )
    return hashlib.sha256(canonical.encode()).hexdigest()


def record_map(output):
    """Index native observations by family, API group and operation label."""
    rows = [line.split("\t") for line in output.splitlines() if line.startswith("OWN\t")]
    result = {(row[1], row[2], row[3]): "\t".join(row) for row in rows}
    assert len(result) == len(rows), "each owner observation must have a unique record key"
    return result


def test_sc_dt_integer_owners_match_native_reference(tmp_path):
    """Audit both 200-row families and compare focused subprocess reference phases."""
    rows_by_family = integer_owner_rows()
    for family, rows in rows_by_family.items():
        assert len(rows) == 200
        assert Counter(row["kind"] for row in rows) == Counter(EXPECTED_KIND_COUNTS)
        assert stable_digest(rows) == INVENTORY_DIGESTS[family]

    for phase in PHASES:
        python_output = run_example(tmp_path, "tests/models/sc_dt_integer_owners_public.py", (phase,))
        native = subprocess.run(
            [
                sys.executable,
                str(ROOT / "examples" / "run_sysc_cpp.py"),
                str(ROOT / "tests" / "native" / "sc_dt_integer_owners_public.cpp"),
                "--",
                phase,
            ],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert native.returncode == 0, native.stdout + native.stderr
        python_records = record_map(python_output)
        native_records = record_map(native.stdout)
        assert python_records == native_records, phase
        for family in INVENTORY_DIGESTS:
            labels = {(group, label) for record_family, group, label in python_records if record_family == family}
            if phase == "constructors":
                assert {("construct", label) for label in EXPECTED_CONSTRUCTOR_LABELS} <= labels
                assert {("assign", label) for label in EXPECTED_ASSIGN_LABELS} <= labels
                assert ("lifetime", "copied_owner") in labels
            elif phase == "compounds":
                assert {("compound", label) for label in EXPECTED_COMPOUND_LABELS} <= labels
                assert {
                    ("increment", label) for label in ("prefix_up", "postfix_up", "prefix_down", "postfix_down")
                } <= labels
            elif phase == "storage":
                assert {("width", str(width)) for width in (1, 32, 137, 191, 257)} <= labels
                assert {("storage", name) for name in ("packed", "raw", "set_packed")} <= labels
                assert {
                    ("view", name) for name in ("bits", "mutable_range", "mutable_call", "const_range", "const_call")
                } <= labels
                assert {
                    ("concat", name) for name in ("get", "set_int64", "set_uint64", "set_signed", "set_unsigned")
                } <= labels
            else:
                assert {
                    ("error", name)
                    for name in (
                        "index_-1",
                        "index_32",
                        "invalid_index_-1",
                        "invalid_index_32",
                        "range_32_0",
                        "invalid_range_32_0",
                        "width_0",
                        "width_-1",
                    )
                } <= labels
        assert "SC_DT_INTEGER_OWNERS_PUBLIC_OK" in python_output
        assert "SC_DT_INTEGER_OWNERS_PUBLIC_OK" in native.stdout
