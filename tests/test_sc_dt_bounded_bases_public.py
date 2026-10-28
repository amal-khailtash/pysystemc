"""Audit the native public APIs of both bounded integer base classes."""

from collections import Counter
import csv
import hashlib
import subprocess
import sys

from .support import ROOT, run_example


INVENTORY_DIGESTS = {
    "sc_dt::sc_int_base": "45ba3793b65708df35352bf5206672eb2ed97437e4e9a9acdd5eeffdaa9c5615",
    "sc_dt::sc_uint_base": "c196749c50e1fd1a631b4fd0178c74924b61ff9612d5a895289893b288c1c9f1",
}
EXPECTED_KIND_COUNTS = {
    "CXXRecordDecl": 4,
    "CXXConstructorDecl": 12,
    "CXXMethodDecl": 78,
    "CXXConversionDecl": 1,
    "CXXDestructorDecl": 1,
    "FunctionTemplateDecl": 2,
}
REQUIRED_GROUP_LABELS = {
    "construct": {
        "default",
        "width",
        "scalar_width",
        "copy",
        "signed",
        "unsigned",
        "signed_subref",
        "unsigned_subref",
        "int_subref",
        "uint_subref",
        "bv",
        "lv",
        "generic",
    },
    "compound": {
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
        "postfix_increment",
        "prefix_decrement",
        "postfix_decrement",
    },
    "observe": {"numeric", "reductions", "strings", "print"},
    "bits": {"set_one_arg", "set_bool", "test"},
    "concat": {"pack", "set_int64", "set_uint64", "set_signed", "set_unsigned"},
    "access": {"bit", "range"},
    "stream": {"scan", "scan_invalid"},
    "invalid_width": {"0", "-1", "65"},
    "lifetime": {"source_released"},
}


def bounded_base_rows():
    """Return each inventory family with mutable status metadata intact."""
    inventory = ROOT / "docs" / "_static" / "public-api-inventory.csv"
    with inventory.open(newline="", encoding="utf-8") as source:
        all_rows = list(csv.DictReader(source))
    return {family: [row for row in all_rows if row["name"].startswith(family)] for family in INVENTORY_DIGESTS}


def stable_digest(rows):
    """Hash declaration identity and location without mutable status fields."""
    canonical = "".join(
        "\t".join(row[field] for field in ("name", "kind", "signature", "source", "line")) + "\n" for row in rows
    )
    return hashlib.sha256(canonical.encode()).hexdigest()


def record_map(output):
    """Index output records by family, method group, and operation label."""
    rows = [line.split("\t") for line in output.splitlines() if line.startswith("BAS\t")]
    result = {(row[1], row[2], row[3]): "\t".join(row) for row in rows}
    assert len(result) == len(rows), "each native base operation must have a unique record key"
    return result


def test_sc_dt_bounded_base_families_match_native_reference(tmp_path):
    """Audit 196 declarations, all overload families, widths and reports.

    The Python and C++ matrices run independently in fresh subprocesses so
    native SystemC report and datatype state never crosses test runs.
    """
    rows_by_family = bounded_base_rows()
    for family, rows in rows_by_family.items():
        assert len(rows) == 98
        assert Counter(row["kind"] for row in rows) == Counter(EXPECTED_KIND_COUNTS)
        assert stable_digest(rows) == INVENTORY_DIGESTS[family]

    python_output = "\n".join(
        run_example(tmp_path, "tests/models/sc_dt_bounded_bases_public.py", (family,)) for family in INVENTORY_DIGESTS
    )
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples" / "run_sysc_cpp.py"),
            str(ROOT / "tests" / "native" / "sc_dt_bounded_bases_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    python_records = record_map(python_output)
    native_records = record_map(native.stdout)
    assert python_records == native_records
    for family in INVENTORY_DIGESTS:
        observed_groups = {(group, label) for record_family, group, label in python_records if record_family == family}
        for group, labels in REQUIRED_GROUP_LABELS.items():
            assert {(group, label) for label in labels} <= observed_groups, (family, group)
        width_labels = {
            label for row_family, group, label in python_records if row_family == family and group == "width"
        }
        assert width_labels == {"1", "19", "59", "63", "64"}
        copied_width_labels = {
            label for row_family, group, label in python_records if row_family == family and group == "width_copy"
        }
        assert copied_width_labels == width_labels

    assert "SC_DT_BOUNDED_BASES_PUBLIC_OK" in python_output
    assert "SC_DT_BOUNDED_BASES_PUBLIC_OK" in native.stdout
