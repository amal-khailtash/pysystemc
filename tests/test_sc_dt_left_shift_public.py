"""Compare the public sc_dt left-shift family with independent native C++."""

import csv
import subprocess
import sys

from .support import ROOT, run_example


def inventory_rows():
    """Read every inventoried sc_dt operator<< declaration in CSV order."""
    path = ROOT / "docs" / "_static" / "public-api-inventory.csv"
    with path.open(newline="", encoding="utf-8") as source:
        return [row for row in csv.DictReader(source) if row["name"] == "sc_dt::operator<<"]


def semantic_records(output):
    """Return operator records indexed by inventory row number.

    Args:
        output: Captured model or native process output.
    Returns:
        Mapping of row id to exact emitted record fields.
    """
    records = {}
    for line in output.splitlines():
        if not line.startswith("LSM\t"):
            continue
        fields = line.split("\t")
        row = int(fields[1])
        assert row not in records, f"duplicate semantic observation for inventory row {row}"
        records[row] = fields
    return records


def test_sc_dt_left_shift_public_rows_match_native(tmp_path):
    """Verify all 67 declarations, wide shift rules, stream output and lifetimes."""
    rows = inventory_rows()
    assert len(rows) == 67

    python_output = run_example(tmp_path, "tests/models/sc_dt_left_shift_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples" / "run_sysc_cpp.py"),
            str(ROOT / "tests" / "native" / "sc_dt_left_shift_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    python_records = semantic_records(python_output)
    native_records = semantic_records(native.stdout)
    inventory_ids = set(range(1, 68))
    auxiliary_ids = set(range(900, 914))
    assert set(python_records) == set(native_records) == inventory_ids | auxiliary_ids
    assert len([row for row in rows if row["signature"].startswith("::std::ostream")]) == 34
    assert len([row for row in rows if row["kind"] == "FunctionTemplateDecl"]) == 1
    assert (
        sum(row["kind"] == "FunctionDecl" and not row["signature"].startswith("::std::ostream") for row in rows) == 32
    )

    for row_number, row in enumerate(rows, 1):
        fields = python_records[row_number]
        expected_kind = (
            "O" if row["signature"].startswith("::std::ostream") or row["kind"] == "FunctionTemplateDecl" else "S"
        )
        assert fields[2] == expected_kind, f"inventory row {row_number}: {row['signature']}"
    assert [python_records[row] for row in sorted(python_records)] == [
        native_records[row] for row in sorted(native_records)
    ]
    assert "SC_DT_LEFT_SHIFT_PUBLIC_OK" in python_output
    assert "SC_DT_LEFT_SHIFT_PUBLIC_OK" in native.stdout
