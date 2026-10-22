"""Compare busy event-list errors and pthread recovery against native C++."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_event_list_busy(tmp_path):
    """Assert assignment/swap errors preserve waiters and later allow mutation."""
    output = run_example(tmp_path, "tests/models/sc_core_event_list_busy.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_event_list_busy.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("EVENT_BUSY_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("EVENT_BUSY_REFERENCE")]
    assert len(actual) == 1 and actual == expected
