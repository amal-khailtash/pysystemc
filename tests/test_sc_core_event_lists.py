"""Compare native event-list public operations with independent C++."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_event_lists(tmp_path):
    """Verify duplicate suppression, expression copies, swaps and base assignment."""
    output = run_example(tmp_path, "tests/models/sc_core_event_lists.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_event_lists.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("EVENT_LIST_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("EVENT_LIST_REFERENCE")]
    assert len(actual) == 1 and actual == expected
