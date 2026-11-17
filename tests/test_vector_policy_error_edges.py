"""Verify vector policy aliases, invalid conversions, and recovery against C++."""

import subprocess
import sys

from .support import ROOT, run_example


def test_vector_policy_aliases_errors_and_live_recovery(tmp_path):
    """Compare valid public policy behavior and require malformed inputs to recover."""
    python_output = run_example(tmp_path, "tests/models/vector_policy_error_edges.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/vector_policy_error_edges.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("VPOLICY\t")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("VPOLICY\t")]
    assert [line for line in python_records if "\terror\t" not in line] == native_records
    assert native_records == [
        "VPOLICY\taliases\t1",
        "VPOLICY\tnested\t1\t7\t3\t11,23,37",
        "VPOLICY\tlive\t1\t1\t29\t73",
    ]
    assert [line.split("\t")[2:] for line in python_records if "\terror\t" in line] == [
        ["member_value_type", "TypeError"],
        ["direct_missing_native_source", "TypeError"],
        ["member_name", "ValueError"],
        ["member_const_to_mutable", "TypeError"],
    ]
