"""Validate integer range-reference families against an independent C++ model."""

import csv
from collections import Counter
import subprocess
import sys

from .support import ROOT, run_example


EXPECTED_GROUPS = {
    "sc_dt::sc_int_subref": 24,
    "sc_dt::sc_int_subref_r": 30,
    "sc_dt::sc_signed_subref": 24,
    "sc_dt::sc_signed_subref_r": 30,
    "sc_dt::sc_uint_subref": 24,
    "sc_dt::sc_uint_subref_r": 29,
    "sc_dt::sc_unsigned_subref": 25,
    "sc_dt::sc_unsigned_subref_r": 30,
}


def subref_inventory_counts():
    """Return declaration counts for the eight integer range-reference groups.

    Returns:
        Mapping from two-part native family name to its CSV declaration count.
    """
    inventory = ROOT / "docs" / "_static" / "public-api-inventory.csv"
    with inventory.open(newline="", encoding="utf-8") as source:
        counts = Counter()
        for row in csv.DictReader(source):
            parts = row["name"].split("::")
            if len(parts) > 1 and parts[0] == "sc_dt" and parts[1] in {name.split("::")[1] for name in EXPECTED_GROUPS}:
                counts["::".join(parts[:2])] += 1
    return dict(counts)


def test_sc_dt_integer_subrefs_match_native_reference(tmp_path):
    """Compare constructors, selections, conversions, mutations, pools and errors.

    Every selected view borrows a live 19-, 63-, 137- or 191-bit parent. The
    inventory assertion scopes the test to all 216 declarations in the eight
    integer subreference groups; compile-time probes check protected APIs.
    """
    assert subref_inventory_counts() == EXPECTED_GROUPS
    assert sum(EXPECTED_GROUPS.values()) == 216

    python_output = run_example(tmp_path, "tests/models/sc_dt_integer_subrefs_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples" / "run_sysc_cpp.py"),
            str(ROOT / "tests" / "native" / "sc_dt_integer_subrefs_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("ISR\t")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("ISR\t")]
    if python_records != native_records:
        for index, (python_record, native_record) in enumerate(zip(python_records, native_records)):
            if python_record != native_record:
                raise AssertionError(f"record {index} differs:\nPython: {python_record}\nNative: {native_record}")
        raise AssertionError(f"record counts differ: Python={len(python_records)} native={len(native_records)}")

    operation_counts = Counter(line.split("\t")[3] for line in python_records)
    expected_operation_counts = {
        "access": 8,
        "copy_ctor": 8,
        "read": 12,
        "bitwidth": 4,
        "reverse": 4,
        "assign_mutable": 8,
        "assign_const": 8,
        "assign_cstr": 8,
        "assign_int": 8,
        "assign_uint": 8,
        "assign_long": 8,
        "assign_ulong": 8,
        "assign_int64": 8,
        "assign_uint64": 8,
        "assign_double": 8,
        "assign_signed": 8,
        "assign_unsigned": 8,
        "assign_int_base": 6,
        "assign_uint_base": 6,
        "assign_sc_uint_generic": 2,
        "assign_sc_int_generic": 2,
        "assign_generic_concat": 8,
        "assign_bv": 4,
        "assign_lv": 4,
        "assign_cross_const": 4,
        "scan": 24,
        "concat_int64": 8,
        "concat_uint64": 8,
        "concat_signed": 8,
        "concat_unsigned": 8,
        "pool": 8,
        "copy_write": 8,
        "copy_lifetime": 8,
        "invalid_range": 20,
    }
    assert operation_counts == expected_operation_counts
    assert len(python_records) == 268
    assert not any(line.startswith("ISR_GAP\t") for line in python_output.splitlines())

    pool_records = [line.split("\t") for line in python_records if "\tpool\t" in line]
    assert len(pool_records) == 8
    assert all(record[-10:] == ["512", "1", "1", "4", "10", "0", "1", "4", "10", "1"] for record in pool_records)
    error_records = [line.split("\t") for line in python_records if "\tinvalid_range\t" in line]
    assert len(error_records) == 20
    assert all(record[-2] == "1" and record[-1] == "0" for record in error_records)
    reverse_records = [line.split("\t") for line in python_records if "\treverse\t" in line]
    assert len(reverse_records) == 4
    assert all(record[1] in ("sc_dt::sc_signed_subref", "sc_dt::sc_unsigned_subref") for record in reverse_records)
