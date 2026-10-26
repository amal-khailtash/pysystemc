"""Verify public fixed-point bit proxy declarations and native parity."""

import csv
from collections import Counter
import subprocess
import sys

from .support import ROOT, run_example

FAMILIES = {
    "sc_dt::sc_fxnum_bitref_r": Counter(
        {
            ("CXXRecordDecl", "sc_dt::sc_fxnum_bitref_r"): 1,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_bitref_r::dump"): 1,
            ("CXXConversionDecl", "sc_dt::sc_fxnum_bitref_r::operator bool"): 1,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_bitref_r::print"): 1,
            ("CXXConstructorDecl", "sc_dt::sc_fxnum_bitref_r::sc_fxnum_bitref_r"): 1,
        }
    ),
    "sc_dt::sc_fxnum_fast_bitref_r": Counter(
        {
            ("CXXRecordDecl", "sc_dt::sc_fxnum_fast_bitref_r"): 1,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_fast_bitref_r::dump"): 1,
            ("CXXConversionDecl", "sc_dt::sc_fxnum_fast_bitref_r::operator bool"): 1,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_fast_bitref_r::print"): 1,
            ("CXXConstructorDecl", "sc_dt::sc_fxnum_fast_bitref_r::sc_fxnum_fast_bitref_r"): 1,
        }
    ),
    "sc_dt::sc_fxnum_bitref": Counter(
        {
            ("CXXRecordDecl", "sc_dt::sc_fxnum_bitref"): 1,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_bitref::operator&="): 4,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_bitref::operator="): 4,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_bitref::operator^="): 4,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_bitref::operator|="): 4,
            ("CXXConstructorDecl", "sc_dt::sc_fxnum_bitref::sc_fxnum_bitref"): 1,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_bitref::scan"): 1,
        }
    ),
    "sc_dt::sc_fxnum_fast_bitref": Counter(
        {
            ("CXXRecordDecl", "sc_dt::sc_fxnum_fast_bitref"): 1,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_fast_bitref::operator&="): 4,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_fast_bitref::operator="): 4,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_fast_bitref::operator^="): 4,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_fast_bitref::operator|="): 4,
            ("CXXConstructorDecl", "sc_dt::sc_fxnum_fast_bitref::sc_fxnum_fast_bitref"): 1,
            ("CXXMethodDecl", "sc_dt::sc_fxnum_fast_bitref::scan"): 1,
        }
    ),
    "sc_dt::sc_bitref_conv_r": Counter(
        {
            ("ClassTemplateDecl", "sc_dt::sc_bitref_conv_r"): 2,
            ("CXXRecordDecl", "sc_dt::sc_bitref_conv_r"): 1,
            ("ClassTemplatePartialSpecializationDecl", "sc_dt::sc_bitref_conv_r"): 1,
            ("ClassTemplateSpecializationDecl", "sc_dt::sc_bitref_conv_r"): 1,
            ("CXXConversionDecl", "sc_dt::sc_bitref_conv_r::operator bool"): 1,
            ("CXXMethodDecl", "sc_dt::sc_bitref_conv_r::operator!"): 1,
        }
    ),
}


def inventory_matrix():
    """Read exact public declaration rows for fixed bit proxy families.

    Returns:
        A counter keyed by class family and declaration kind/name.
    """
    inventory_path = ROOT / "docs/_static/public-api-inventory.csv"
    with inventory_path.open(newline="") as inventory_file:
        rows = Counter()
        for row in csv.DictReader(inventory_file):
            for family in FAMILIES:
                if row["name"] == family or row["name"].startswith(family + "::"):
                    rows[(family, row["kind"], row["name"])] += 1
    return rows


def run_native(tmp_path, mode=None):
    """Run the standalone C++ reference in an isolated process.

    Args:
        tmp_path: Working directory for this one-shot native kernel.
        mode: Optional bounded native error or fast-width probe.

    Returns:
        Completed subprocess result with native output captured as text.
    """
    command = [
        sys.executable,
        str(ROOT / "examples/run_sysc_cpp.py"),
        str(ROOT / "tests/native/sc_dt_fixed_bitrefs_public.cpp"),
    ]
    if mode is not None:
        command.extend(("--", mode))
    return subprocess.run(command, cwd=tmp_path, capture_output=True, text=True, timeout=60)


def transcript(output):
    """Extract the common FXBIT rows from runner noise.

    Args:
        output: Combined stdout text from either reference implementation.

    Returns:
        The ordered semantic rows emitted by the test model.
    """
    return [line for line in output.splitlines() if line.startswith("FXBIT\t")]


def test_sc_dt_fixed_bitrefs_public(tmp_path):
    """Match 48 fixed proxy declarations and 7 sc_bitref conversion rows."""
    inventory = inventory_matrix()
    for family, expected in FAMILIES.items():
        actual = Counter(
            {(kind, name): count for (row_family, kind, name), count in inventory.items() if row_family == family}
        )
        assert actual == expected
    assert sum(sum(rows.values()) for rows in FAMILIES.values()) == 55

    python_output = run_example(tmp_path, "tests/models/sc_dt_fixed_bitrefs_public.py")
    native = run_native(tmp_path)
    assert native.returncode == 0, native.stdout + native.stderr
    python_rows = transcript(python_output)
    native_rows = transcript(native.stdout)
    assert python_rows == native_rows

    rows = [row.split("\t") for row in python_rows]
    assert Counter(row[1] for row in rows) == Counter(
        {
            "width": 2,
            "precision": 1,
            "view": 10,
            "copy": 2,
            "scan": 2,
            "access": 3,
            "op": 32,
            "conv": 2,
            "proxy_access": 1,
        }
    )
    widths = {row[2]: int(row[3]) for row in rows if row[1] == "width"}
    assert widths == {"fixed": 137, "fast": 64}
    views = {(row[2], int(row[3])): row for row in rows if row[1] == "view"}
    assert "idx = -68" in views[("fixed", 0)][6]
    assert "idx = -32" in views[("fast", 0)][6]
    accesses = {row[2]: row for row in rows if row[1] == "access"}
    assert set(accesses) == {"fixed", "fast", "fast137"}
    for row in accesses.values():
        initial, item_write, method_write = row[4:7]
        assert item_write == str(1 - int(initial))
        assert method_write == initial
    assert accesses["fast137"][4:7] == ["1", "0", "1"]
    fixed_precision, fast_precision = next(row[2:] for row in rows if row[1] == "precision")
    assert ".01" in fixed_precision
    assert ".000" in fast_precision
    proxy_access = next(row for row in rows if row[1] == "proxy_access")
    access_traits = dict(zip(proxy_access[2::2], proxy_access[3::2], strict=True))
    assert access_traits == {
        "fixed_mutable_is_constructible": "0",
        "fixed_readonly_is_constructible": "0",
        "fast_mutable_is_constructible": "0",
        "fast_readonly_is_constructible": "0",
        "fixed_mutable_copyable": "1",
        "fixed_readonly_copyable": "1",
        "fast_mutable_copyable": "1",
        "fast_readonly_copyable": "1",
        "fixed_mutable_destructible": "1",
        "fixed_readonly_destructible": "1",
        "fast_mutable_destructible": "1",
        "fast_readonly_destructible": "1",
    }


def test_sc_dt_fixed_bitrefs_native_reports_and_fast_limit(tmp_path):
    """Match valid out-of-range reports and wide fast-number native precision."""
    for mode in ("bad-index-fixed", "bad-index-fast"):
        python_output = run_example(tmp_path, "tests/models/sc_dt_fixed_bitrefs_public.py", (mode,))
        native = run_native(tmp_path, mode)
        assert native.returncode == 0, native.stdout + native.stderr
        python_rows = transcript(python_output)
        native_rows = transcript(native.stdout)
        assert python_rows == native_rows
        assert len(python_rows) == 1
        assert "index out of range" in python_rows[0]

    python_output = run_example(tmp_path, "tests/models/sc_dt_fixed_bitrefs_public.py", ("fast-wide",))
    native = run_native(tmp_path, "fast-wide")
    assert native.returncode == 0, native.stdout + native.stderr
    assert transcript(python_output) == transcript(native.stdout)
    fast_bin = transcript(python_output)[0].split("\t", 2)[2]
    assert fast_bin.endswith(".0000000000000000000000000000000000000")
