"""Validate integer bit-reference proxies against native SystemC output."""

import csv
from collections import Counter
import subprocess
import sys

from .support import ROOT, run_example


EXPECTED_GROUPS = {
    "sc_dt::sc_int_bitref": 13,
    "sc_dt::sc_int_bitref_r": 15,
    "sc_dt::sc_signed_bitref": 14,
    "sc_dt::sc_signed_bitref_r": 16,
    "sc_dt::sc_uint_bitref": 13,
    "sc_dt::sc_uint_bitref_r": 15,
    "sc_dt::sc_unsigned_bitref": 14,
    "sc_dt::sc_unsigned_bitref_r": 16,
}


def bitref_inventory_counts():
    """Return declaration counts for the eight integer bit-reference groups.

    Returns:
        A mapping from the two-part native family name to its CSV row count.
    """
    inventory = ROOT / "docs" / "_static" / "public-api-inventory.csv"
    with inventory.open(newline="", encoding="utf-8") as source:
        rows = csv.DictReader(source)
        counts = Counter()
        for row in rows:
            parts = row["name"].split("::")
            if len(parts) > 1 and parts[0] == "sc_dt" and parts[1] in {name.split("::")[1] for name in EXPECTED_GROUPS}:
                counts["::".join(parts[:2])] += 1
    return dict(counts)


def test_sc_dt_integer_bitrefs_match_native_reference(tmp_path):
    """Compare mutable/const access, truth conversion, pools and errors with C++.

    Each selected proxy comes from a live parent at 19, 63, 137 or 191 bits.
    The CSV count check keeps the test scoped to all 116 declared integer
    bit-reference rows; protected constructors and initialization are inspected
    through native compile-time access probes instead of direct Python creation.
    """
    assert bitref_inventory_counts() == EXPECTED_GROUPS
    assert sum(EXPECTED_GROUPS.values()) == 116

    python_output = run_example(tmp_path, "tests/models/sc_dt_integer_bitrefs_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples" / "run_sysc_cpp.py"),
            str(ROOT / "tests" / "native" / "sc_dt_integer_bitrefs_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("IBR\t")]
    gap_records = [line for line in python_output.splitlines() if line.startswith("IBR_GAP\t")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("IBR\t")]
    if python_records != native_records:
        for index, (python_record, native_record) in enumerate(zip(python_records, native_records)):
            if python_record != native_record:
                raise AssertionError(f"record {index} differs:\nPython: {python_record}\nNative: {native_record}")
        raise AssertionError(f"record counts differ: Python={len(python_records)} native={len(native_records)}")
    pool_records = [line.split("\t") for line in python_records if "\tpool\t" in line]
    assert len(pool_records) == 8
    assert all(record[-7:] == ["512", "1", "1", "0", "1", "0", "1"] for record in pool_records)
    assert len(python_records) == 224
    expected_truth_families = {
        "sc_dt::sc_int_bitref": (19, 63),
        "sc_dt::sc_uint_bitref": (19, 63),
        "sc_dt::sc_signed_bitref": (137, 191),
        "sc_dt::sc_unsigned_bitref": (137, 191),
    }
    truth_records = {tuple(line.split("\t")[1:]) for line in python_records if "\ttruth\t" in line}
    assert truth_records == {
        (family, str(width), "truth", proxy, str(value), str(value), str(1 - value))
        for family, widths in expected_truth_families.items()
        for width in widths
        for proxy in ("mutable", "const")
        for value in (0, 1)
    }
    expected_read_families = {f"sc_dt::{name}_bitref_r" for name in ("sc_int", "sc_uint", "sc_signed", "sc_unsigned")}
    assert len(gap_records) == 8
    observed_gaps = {tuple(line.split("\t")[1:]) for line in gap_records}
    assert observed_gaps == {
        (
            family,
            str(width),
            "const_proxy_type",
            f"cppyy_type={family.removesuffix('_r')}",
            f"native_static_type={family}",
        )
        for family in expected_read_families
        for width in (19, 63)
        if "sc_int" in family or "sc_uint" in family
    } | {
        (
            family,
            str(width),
            "const_proxy_type",
            f"cppyy_type={family.removesuffix('_r')}",
            f"native_static_type={family}",
        )
        for family in expected_read_families
        for width in (137, 191)
        if "sc_signed" in family or "sc_unsigned" in family
    }
