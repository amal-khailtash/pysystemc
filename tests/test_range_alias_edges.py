"""Verify legacy range aliases and copied fixed-point vectors against native SystemC."""

from collections import Counter
import subprocess
import sys

from .support import ROOT, run_example


def test_range_aliases_and_fixed_vector_copies_match_native(tmp_path):
    """Compare alias results across fixed, arbitrary, integer and concat views."""
    python_output = run_example(tmp_path, "tests/models/range_alias_edges.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/range_alias_edges.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("RANGEALIAS\t")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("RANGEALIAS\t")]
    assert python_records == native_records
    assert len(python_records) == 23

    rows = [line.split("\t") for line in python_records]
    assert [row[1:3] for row in rows if row[1] in {"fixed", "fast", "signed_owner", "unsigned_owner"}] == [
        ["fixed", "mutable_before"],
        ["fixed", "const_before"],
        ["fixed", "mutable_after"],
        ["fixed", "const_after"],
        ["fast", "mutable_before"],
        ["fast", "const_before"],
        ["fast", "mutable_after"],
        ["fast", "const_after"],
        ["signed_owner", "before"],
        ["signed_owner", "after"],
        ["unsigned_owner", "before"],
        ["unsigned_owner", "after"],
    ]
    assert (
        next(row for row in rows if row[1:3] == ["fixed", "mutable_before"])[3:]
        != next(row for row in rows if row[1:3] == ["fixed", "mutable_after"])[3:]
    )
    assert next(row for row in rows if row[1] == "vector_copy")[2] == "137"
    assert Counter(row[1] for row in rows) == Counter(
        {
            "fixed": 4,
            "fast": 4,
            "vector_copy": 1,
            "signed_owner": 2,
            "unsigned_owner": 2,
            "int_range": 2,
            "int_bit": 2,
            "uint_range": 2,
            "uint_bit": 2,
            "concat": 2,
        }
    )
