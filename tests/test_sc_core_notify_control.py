"""Compare notification/control semantics against an independent native model."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_notify_control(tmp_path):
    """Verify free notify callback timing/deltas, initialization and pause/resume."""
    output = run_example(tmp_path, "tests/models/sc_core_notify_control.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_notify_control.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [
        line for line in output.splitlines() if line.startswith(("NOTIFY_CONTROL_REFERENCE", "NOTIFY_FINAL_REFERENCE"))
    ]
    expected = [
        line
        for line in native.stdout.splitlines()
        if line.startswith(("NOTIFY_CONTROL_REFERENCE", "NOTIFY_FINAL_REFERENCE"))
    ]
    assert len(actual) == 5
    assert actual == expected
