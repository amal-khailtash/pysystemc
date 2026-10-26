"""Audit protected fixed-number bases and compare exact behavior with native SystemC."""

from collections import Counter
import csv
import hashlib
import subprocess
import sys

from .support import ROOT, run_example

CLASSES = ("sc_dt::sc_fxnum", "sc_dt::sc_fxnum_fast")
INVENTORY_HASHES = {
    "sc_dt::sc_fxnum": "cff69aebf118aee9e1402767cb0bc8d51adac96ab142594dff5a45d3b950ad8c",
    "sc_dt::sc_fxnum_fast": "a27a6f211ff47739f400d8987e15dba2a7d178c8af5b587643160c329dac19fd",
}
HASH_FIELDS = ("id", "name", "kind", "signature", "source", "line")
SOURCES = {
    "int",
    "unsigned int",
    "long",
    "unsigned long",
    "float",
    "double",
    "const char *",
    "const sc_dt::sc_fxval &",
    "const sc_dt::sc_fxval_fast &",
    "const sc_dt::sc_fxnum &",
    "const sc_dt::sc_fxnum_fast &",
    "sc_dt::int64",
    "sc_dt::uint64",
    "const sc_dt::sc_int_base &",
    "const sc_dt::sc_uint_base &",
    "const sc_dt::sc_signed &",
    "const sc_dt::sc_unsigned &",
}


def inventory_rows():
    """Return each number family while excluding private default/copy constructors."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        rows = list(csv.DictReader(source))
    selected = {}
    for class_name in CLASSES:
        private = {"void ()", f"void (const {class_name} &)"}
        selected[class_name] = [
            row
            for row in rows
            if (row["name"] == class_name or row["name"].startswith(class_name + "::"))
            and not (row["kind"] == "CXXConstructorDecl" and row["signature"] in private)
        ]
    return selected


def records(output):
    """Select deterministic records shared by the Python and native models."""
    return [line for line in output.splitlines() if line.startswith("FXN\t")]


def test_sc_dt_fxnums_inventory_and_native_parity(tmp_path):
    """Check 171 inventory rows, constructor access, mutations, state and precision."""
    rows = inventory_rows()
    for class_name, declarations in rows.items():
        assert len(declarations) == 171
        canonical = "".join("\t".join(row[field] for field in HASH_FIELDS) + "\n" for row in declarations)
        assert hashlib.sha256(canonical.encode()).hexdigest() == INVENTORY_HASHES[class_name]
        assert Counter(row["kind"] for row in declarations) == Counter(
            {
                "CXXRecordDecl": 10,
                "CXXConstructorDecl": 18,
                "CXXMethodDecl": 141,
                "CXXConversionDecl": 1,
                "CXXDestructorDecl": 1,
            }
        )
        constructors = [row for row in declarations if row["kind"] == "CXXConstructorDecl"]
        assert len(constructors) == 18
        assert {
            row["signature"][row["signature"].index("(") + 1 : row["signature"].index(", const sc_dt::sc_fxtype_params")]
            for row in constructors
            if ", const sc_dt::sc_fxtype_params" in row["signature"]
        } == SOURCES
        assert sum(", const sc_dt::sc_fxtype_params" in row["signature"] for row in constructors) == 17
        assert sum(row["signature"].startswith("void (const sc_dt::sc_fxtype_params &") for row in constructors) == 1

    python_output = run_example(tmp_path, "tests/models/sc_dt_fxnums_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_fxnums_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records, native_records = records(python_output), records(native.stdout)
    assert sorted(python_records) == sorted(native_records)
    assert "SC_DT_FXNUMS_PUBLIC_OK" in python_output
    assert "SC_DT_FXNUMS_PUBLIC_OK" in native.stdout

    grouped = {}
    for class_name in CLASSES:
        family = class_name.removeprefix("sc_dt::")
        selected = [line.split("\t") for line in python_records if line.split("\t")[2] == family]
        grouped[family] = selected
        assert Counter(line[1] for line in selected) == Counter(
            {
                "CTOR": 18,
                "ASSIGN": 17,
                "add": 17,
                "subtract": 17,
                "multiply": 17,
                "divide": 17,
                "prefix": 2,
                "postfix": 2,
                "proxy": 2,
                "borrow": 1,
                "format": 1,
                "value": 1,
                "precision": 2,
                "lifetime": 1,
            }
        )
        assert {line[3] for line in selected if line[1] == "ASSIGN"} == SOURCES
        for operation in ("add", "subtract", "multiply", "divide"):
            assert {line[3] for line in selected if line[1] == operation} == SOURCES
        assert {line[3] for line in selected if line[1] == "precision"} == {"wide137", "wide191"}
        assert {line[3] for line in selected if line[1] == "proxy"} == {"bit", "range"}
        assert next(line[4].rsplit("|", 1)[1] for line in selected if line[1] == "value") == "1"
        borrow = next(line[4] for line in selected if line[1] == "borrow")
        assert borrow == "1|1"
        assert next(line[4] for line in selected if line[1] == "lifetime") == "1"

    normal_wide = {
        line[3]: line[4].split("|")
        for line in grouped["sc_fxnum"]
        if line[1] == "precision"
    }
    fast_wide = {
        line[3]: line[4].split("|")
        for line in grouped["sc_fxnum_fast"]
        if line[1] == "precision"
    }
    assert normal_wide["wide137"][2] == "137" and normal_wide["wide191"][2] == "191"
    assert fast_wide["wide137"][2] == "137" and fast_wide["wide191"][2] == "191"
    assert normal_wide["wide137"][1] != fast_wide["wide137"][1]
