"""Check first-party channel reset/kill completion against native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_blocking_completion(tmp_path):
    """Compare direct and native-functor routes with reset/kill reference rows."""
    python_outputs = []
    for route in ("direct", "functor"):
        run_dir = tmp_path / route
        run_dir.mkdir()
        python_outputs.append(run_example(run_dir, "tests/models/sc_core_blocking_completion.py", (route,)))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_blocking_completion.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [
        sorted(line for line in output.splitlines() if line.startswith("BLOCKING_REFERENCE|"))
        for output in python_outputs
    ]
    native_records = sorted(line for line in native.stdout.splitlines() if line.startswith("BLOCKING_REFERENCE|"))
    assert python_records[0] == python_records[1] == native_records
    assert all("SC_CORE_BLOCKING_COMPLETION_OK" in output for output in python_outputs)
