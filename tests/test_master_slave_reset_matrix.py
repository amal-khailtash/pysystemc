"""Compare master/slave reset and kill unwind behavior with a native reference."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def case_records(output):
    """Return normalized lifecycle records in deterministic order."""
    return sorted(line for line in output.splitlines() if line.startswith("MASTER_SLAVE_RESET_CASE "))


def test_master_slave_blocking_unwind_matrix_matches_native_reference(tmp_path):
    """Assert callback unwind order, output sentinels, times, counts, and handles."""
    wait_slices = ((0, 3), (3, 6), (6, 9))
    python_records = []
    for wait_start, wait_end in wait_slices:
        arguments = (str(wait_start), str(wait_end))
        output = run_example(tmp_path, "tests/models/master_slave_reset_matrix.py", arguments)
        assert f"MASTER_SLAVE_RESET_MATRIX_OK {240} " in output
        assert len(case_records(output)) == 240
        python_records.extend(case_records(output))

    assert len(python_records) == 720
    env = dict(os.environ)
    env["SYSTEMC_HOME"] = os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc"))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/master_slave_reset_matrix.cpp"),
        ],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "MASTER_SLAVE_RESET_MATRIX_NATIVE_OK 32" in native.stdout
    native_records = case_records(native.stdout)
    python_output_events = [
        line
        for line in python_records
        if line.split()[3] in ("get-output", "peek-output") and line.split()[4] == "event"
    ]
    assert len(python_output_events) == 32
    assert len(native_records) == 32
    assert sorted(python_output_events) == native_records
