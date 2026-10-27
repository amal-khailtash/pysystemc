"""Compare fixed-point value declarations and typed behavior with native SystemC."""

from collections import Counter
import csv
import hashlib
import math
import subprocess
import sys

from .support import ROOT, run_example

INVENTORY_FIELDS = ("id", "name", "kind", "signature", "source", "line")
INVENTORY_HASHES = {
    "sc_dt::sc_fxval": "4de1f99c8ae3a31c0aa6c4b278010ca357329f0fa311f92dc05360609874059d",
    "sc_dt::sc_fxval_fast": "36b6ac6f248cbcca28fc453dbe22b3a278a9347ec57925d08266970e1ff82243",
}


def fixed_rows():
    """Read all declarations owned by the arbitrary and fast scalar classes."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        rows = list(csv.DictReader(source))
    return {
        type_name: [row for row in rows if row["name"] == type_name or row["name"].startswith(type_name + "::")]
        for type_name in INVENTORY_HASHES
    }


def row_hash(rows):
    """Hash declaration identity and signature, excluding mutable review status."""
    canonical = "".join("\t".join(row[field] for field in INVENTORY_FIELDS) + "\n" for row in rows)
    return hashlib.sha256(canonical.encode()).hexdigest()


def semantic_records(output):
    """Select deterministic datatype records from captured process output."""
    return [line for line in output.splitlines() if line.startswith("FX")]


def test_sc_dt_fxvalues_public_inventory_and_native_parity(tmp_path):
    """Audit both 164-row classes, exact typed operations, observers and precision."""
    rows = fixed_rows()
    for type_name, declarations in rows.items():
        assert len(declarations) == 164
        assert row_hash(declarations) == INVENTORY_HASHES[type_name]
        assert Counter(row["kind"] for row in declarations) == Counter(
            {
                "CXXRecordDecl": 9,
                "CXXConstructorDecl": 20,
                "CXXMethodDecl": 133,
                "CXXConversionDecl": 1,
                "CXXDestructorDecl": 1,
            }
        )

    python_output = run_example(tmp_path, "tests/models/sc_dt_fxvalues_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_fxvalues_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=90,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = semantic_records(python_output)
    native_records = semantic_records(native.stdout)
    assert sorted(python_records) == sorted(native_records)

    counts = Counter(line.split("\t", 1)[0] for line in python_records)
    assert counts == Counter(
        {
            "FXCTOR": 36,
            "FXASSIGN": 34,
            "FXCOMPOUND": sum(
                len(
                    {
                        row["signature"][row["signature"].index("(") + 1 : row["signature"].rindex(")")]
                        for row in declarations
                        if row["name"].endswith("::" + method)
                    }
                )
                for declarations in rows.values()
                for method in (
                    "operator+=",
                    "operator-=",
                    "operator*=",
                    "operator/=",
                    "operator<<=",
                    "operator>>=",
                )
            ),
            "FXBIN": 128,
            "FXOP": 120,
            "FXLIFE": 1,
            "FXOBS": 2,
            "FXFMT": 2,
            "FXDUMP": 2,
            "FXPREC": 1,
            "FXPY": 6,
        }
    )
    assert "SC_DT_FXVALUES_PUBLIC_OK" in python_output
    assert "SC_DT_FXVALUES_PUBLIC_OK" in native.stdout

    precision = next(line.split("\t")[-1] for line in python_records if line.startswith("FXPREC\t"))
    fields = precision.split("|")
    assert fields[0] != fields[1]
    assert math.isclose(float(fields[2]), float(fields[3]), rel_tol=1e-15, abs_tol=0)
