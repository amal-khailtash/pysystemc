"""Audit 548 constrained fixed-template declarations with independent native parity."""

from collections import Counter
import csv
import hashlib
import subprocess
import sys

from .support import ROOT, run_example

FAMILIES = ("sc_fixed", "sc_ufixed", "sc_fixed_fast", "sc_ufixed_fast")
INVENTORY_SHA256 = "069cafcc507393cffc1ac2f3618147214ee4e0b8c1f77deb9d716268f3224127"
SOURCES = {
    "int",
    "unsigned_int",
    "long",
    "unsigned_long",
    "float",
    "double",
    "text",
    "fxval",
    "fxval_fast",
    "fxnum",
    "fxnum_fast",
    "int64",
    "uint64",
    "int_base",
    "uint_base",
    "signed",
    "unsigned",
}
POLICIES = {"wide137", "wide191", "round_saturate", "truncate_wrap", "negative_iwl", "integer_iwl"}


def inventory_rows():
    """Return the frozen class/member shape without mutable support metadata."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        return [
            (row["name"], row["kind"], row["source"], row["signature"])
            for row in csv.DictReader(source)
            if row["name"].removeprefix("sc_dt::").split("::", 1)[0] in FAMILIES
        ]


def records(output):
    """Select exact native-comparable constructor and mutation observations."""
    return [line for line in output.splitlines() if line.startswith("FTM\t")]


def test_sc_dt_fixed_templates_inventory_and_native_parity(tmp_path):
    """Cover all 548 declared rows, native policies, types and observer ownership."""
    rows = inventory_rows()
    assert len(rows) == 548
    assert Counter(name.removeprefix("sc_dt::").split("::", 1)[0] for name, _, _, _ in rows) == Counter(
        dict.fromkeys(FAMILIES, 137)
    )
    canonical = "".join("\t".join(row) + "\n" for row in rows)
    assert hashlib.sha256(canonical.encode()).hexdigest() == INVENTORY_SHA256
    python_output = run_example(tmp_path, "tests/models/sc_dt_fixed_templates_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_fixed_templates_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records, native_records = records(python_output), records(native.stdout)
    assert python_records == native_records
    assert len(python_records) == 748
    split = [line.split("\t") for line in python_records]
    for family in FAMILIES:
        selected = [row for row in split if row[2] == family]
        assert Counter(row[1] for row in selected) == Counter(
            {
                "traits": 1,
                "ctor": 20,
                "ctor_cast": 17,
                "ctor_public": 17,
                "ctor_public_cast": 17,
                "assign": 18,
                "add": 17,
                "subtract": 17,
                "multiply": 17,
                "divide": 17,
                "and": 2,
                "or": 2,
                "xor": 2,
                "left": 1,
                "right": 1,
                "preinc": 1,
                "postinc": 1,
                "predec": 1,
                "postdec": 1,
                "observer_lifetime": 2,
                "lifetime": 1,
                "error": 1,
                "policy_ctor": 6,
                "policy_assign": 6,
                "python_integer": 1,
            }
        )
        assert {row[3] for row in selected if row[1] == "ctor"} == SOURCES | {"observer", "cast_observer", "copy"}
        for operation in ("ctor_cast", "ctor_public", "ctor_public_cast", "add", "subtract", "multiply", "divide"):
            assert {row[3] for row in selected if row[1] == operation} == SOURCES
        assert {row[3] for row in selected if row[1] == "assign"} == SOURCES | {"copy"}
        assert {row[3] for row in selected if row[1] == "policy_ctor"} == POLICIES
        assert {row[3] for row in selected if row[1] == "policy_assign"} == POLICIES
        assert f"FTM\terror\t{family}\ttext\t1\t1" in python_records
        assert f"FTM\tobserver_lifetime\t{family}\tnumbers_gone\t1" in python_records
        assert f"FTM\tobserver_lifetime\t{family}\tobserver_gone\t1" in python_records
    assert "SC_DT_FIXED_TEMPLATES_PUBLIC_OK" in python_output
