"""Audit every concatref declaration against independent native semantic records."""

from collections import Counter
import csv
import hashlib
import subprocess
import sys

from .support import ROOT, run_example

INVENTORY_SHA256 = "3726ba5c47978e30bbc7d112a07943ad6093cf85089baf2c24afb7d7294b280d"
ASSIGNMENTS = {
    "int",
    "long",
    "int64",
    "unsigned_int",
    "unsigned_long",
    "uint64",
    "concatref",
    "signed",
    "unsigned",
    "text",
    "bv",
    "lv",
}
CONCAT_SET = {"int64", "uint64", "signed", "unsigned"}


def inventory_rows():
    """Return exactly the frozen class, flags and member overload inventory."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        return [
            (row["name"], row["kind"], row["source"], row["signature"])
            for row in csv.DictReader(source)
            if row["name"] == "sc_dt::sc_concatref" or row["name"].startswith("sc_dt::sc_concatref::")
        ]


def records(output):
    """Select deterministic member, error and ownership observations."""
    return [line for line in output.splitlines() if line.startswith("CCR\t")]


def test_sc_dt_concatref_inventory_and_native_parity(tmp_path):
    """Cover 59 declarations and compare all public results with fresh native C++."""
    rows = inventory_rows()
    assert len(rows) == 59
    canonical = "".join("\t".join(row) + "\n" for row in rows)
    assert hashlib.sha256(canonical.encode()).hexdigest() == INVENTORY_SHA256
    python_output = run_example(tmp_path, "tests/models/sc_dt_concatref_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_dt_concatref_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records, native_records = records(python_output), records(native.stdout)
    assert python_records == native_records
    assert len(python_records) == 57
    split = [line.split("\t") for line in python_records]
    assert {row[2] for row in split if row[1] == "assign"} == ASSIGNMENTS
    assert {(row[2], row[3]) for row in split if row[1] == "set"} == {
        (label, str(low)) for label in CONCAT_SET for low in (0, 9)
    }
    assert Counter(row[1] for row in split) == Counter(
        {
            "capacity": 1,
            "const": 1,
            "convert": 2,
            "string": 2,
            "value": 2,
            "output": 2,
            "unary": 2,
            "assign": 12,
            "set": 8,
            "reduce": 4,
            "scan": 1,
            "error": 1,
            "overlap": 1,
            "logic": 1,
            "pack": 10,
            "clear": 2,
            "reinit": 1,
            "shell_wrap": 1,
            "value_wrap": 1,
            "destruction": 1,
            "owners_gone": 1,
        }
    )
    assert "CCR\terror\t1\t1" in python_records
    assert "CCR\toverlap\t60\t165\t15525" in python_records
    assert "CCR\tlogic\t10\t1\t662\t662" in python_records
    assert "CCR\towners_gone\t42300\t42300\t16\t42300\t17\t-42300\t17\t88771" in python_records
    assert "SC_DT_CONCATREF_PUBLIC_OK" in python_output
