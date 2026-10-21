"""Compare stop-mode configuration against an independent native model."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_stop_mode(tmp_path):
    """Verify stop-mode mutation, after-start errors and simulator status."""
    output = run_example(tmp_path, "tests/models/sc_core_stop_mode.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_stop_mode.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("STOP_MODE_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("STOP_MODE_REFERENCE")]
    assert len(actual) == 1
    assert actual == expected
