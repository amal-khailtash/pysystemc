"""Compare the public sc_proxy surface and Python behavior with native SystemC."""

from collections import Counter
import csv
import hashlib
import subprocess
import sys

from .support import ROOT, run_example


SC_PROXY_SHA256 = "e7d8f0003c2233202c8bb7d8f87e9d8fef3bec268150d5786e49c4bda41cf876"
SC_PROXY_LV_SHA256 = "72b3c8782784f4007f5ab9f3137fb9242f29108e1697e4f6fa635aa5f4ee40f1"
SOURCE_GROUPS = {
    "submodules/systemc/src/sysc/datatypes/bit/sc_proxy.h": (398, SC_PROXY_SHA256),
    "submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h": (86, SC_PROXY_LV_SHA256),
}
SOURCE_LABELS = {
    "char_pointer",
    "bool_pointer",
    "logic_pointer",
    "unsigned",
    "signed",
    "uint_base",
    "int_base",
    "unsigned_long",
    "long",
    "unsigned_int",
    "int",
    "uint64",
    "int64",
}


def inventory_groups():
    """Return frozen declaration rows grouped by the two proxy source headers.

    Returns:
        Mapping from native source path to exact name, kind and signature rows.
    """
    inventory = ROOT / "docs" / "_static" / "public-api-inventory.csv"
    groups = {source: [] for source in SOURCE_GROUPS}
    with inventory.open(newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            if row["source"] not in groups:
                continue
            name = row["name"]
            if name == "sc_dt::sc_proxy" or name.startswith("sc_dt::sc_proxy::"):
                groups[row["source"]].append(
                    (row["name"], row["kind"], row["source"], row["signature"])
                )
    return groups


def proxy_records(output):
    """Select semantic records from one proxy model process."""
    return [line for line in output.splitlines() if line.startswith("SDP\t")]


def test_sc_dt_proxy_inventory_and_native_parity(tmp_path):
    """Check 484 declarations, all source overloads, borrowed views and lifetimes."""
    groups = inventory_groups()
    assert set(groups) == set(SOURCE_GROUPS)
    for source, rows in groups.items():
        expected_count, expected_digest = SOURCE_GROUPS[source]
        assert len(rows) == expected_count
        canonical = "".join("\t".join(row) + "\n" for row in rows)
        assert hashlib.sha256(canonical.encode()).hexdigest() == expected_digest
    assert sum(count for count, _ in SOURCE_GROUPS.values()) == 484

    python_output = run_example(tmp_path, "tests/models/sc_dt_proxy_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples" / "run_sysc_cpp.py"),
            str(ROOT / "tests" / "native" / "sc_dt_proxy_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = proxy_records(python_output)
    native_records = proxy_records(native.stdout)
    if python_records != native_records:
        for index, (python_record, native_record) in enumerate(zip(python_records, native_records)):
            if python_record != native_record:
                raise AssertionError(
                    f"record {index} differs:\nPython: {python_record}\nNative: {native_record}"
                )
        raise AssertionError(
            f"record counts differ: Python={len(python_records)} native={len(native_records)}"
        )

    records = [line.split("\t") for line in python_records]
    assert Counter(row[2] for row in records) == Counter(
        {
            "source": 39,
            "observe": 2,
            "format_report": 1,
            "transform": 4,
            "view": 4,
            "view_copy": 2,
            "io": 2,
            "crtp": 1,
            "lifetime": 1,
            "report": 1,
        }
    )
    assert {row[3] for row in records if row[2] == "source"} == SOURCE_LABELS
    assert {row[1] for row in records if row[2] == "source"} == {"bv", "lv", "custom"}
    for family in ("bv", "lv", "custom"):
        assert {row[3] for row in records if row[1] == family and row[2] == "source"} == SOURCE_LABELS
    assert "SDP\tcustom\tlifetime\tvirtual_delete\t1" in python_records
    assert any(
        row[:4] == ["SDP", "bv", "report", "negative_shift"] and row[4] == "1"
        for row in records
    )
