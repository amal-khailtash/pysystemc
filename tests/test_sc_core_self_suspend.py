"""Compare Python current-process suspension lifecycle to native SystemC."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("mode", ("resume", "reset", "kill"))
def test_sc_core_self_suspend(tmp_path, mode):
    """Verify self-suspend resume, reset restart and kill cleanup at native times."""
    output = run_example(tmp_path, "tests/models/sc_core_self_suspend.py", (mode,))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_self_suspend.cpp"),
            "--",
            mode,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("SELF_SUSPEND_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("SELF_SUSPEND_REFERENCE")]
    assert actual == expected
    actual_events = [line for line in output.splitlines() if line.startswith("SELF_SUSPEND_EVENT_REFERENCE")]
    expected_events = [line for line in native.stdout.splitlines() if line.startswith("SELF_SUSPEND_EVENT_REFERENCE")]
    assert actual_events == expected_events
