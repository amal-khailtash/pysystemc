"""Compare public event queue and interface contracts against native C++."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_event_queue(tmp_path):
    """Verify equal-time delta deliveries, cancel-all recovery and interface calls."""
    output = run_example(tmp_path, "tests/models/sc_core_event_queue.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_event_queue.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("EVENT_QUEUE_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("EVENT_QUEUE_REFERENCE")]
    assert len(actual) == 4 and actual == expected
