"""Check original B3 teardown after native reset/kill callback unwind."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize(
    "wait_kind", ("time", "event", "or", "and", "timed-event", "timed-or", "timed-and", "cycle", "count")
)
def test_interface_teardown_after_reset_kill(tmp_path, wait_kind):
    """Assert six paths clean up and terminate before native deletion and owner GC."""
    assert "RESET_TEARDOWN_OK 6 4 ns" in run_example(tmp_path, "tests/models/reset_teardown.py", (wait_kind,))


def test_reset_teardown_native_reference(tmp_path):
    """Compare exact callback/worker unwind and deletion records with native FIFO."""
    output = run_example(tmp_path, "tests/models/reset_teardown.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/reset_teardown.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    records = [line for line in output.splitlines() if line.startswith("TEARDOWN_REFERENCE")]
    assert len(records) == 9
    assert records == [line for line in native.stdout.splitlines() if line.startswith("TEARDOWN_REFERENCE")]
