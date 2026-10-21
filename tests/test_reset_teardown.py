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
    records = [
        line for line in output.splitlines() if line.startswith(("TEARDOWN_REFERENCE fifo ", "TEARDOWN_REFERENCE all "))
    ]
    assert len(records) == 9
    assert records == [line for line in native.stdout.splitlines() if line.startswith("TEARDOWN_REFERENCE")]


@pytest.mark.parametrize("datatype", ("long_long", "wide137", "signed128"))
@pytest.mark.parametrize(
    "wait_kind", ("time", "event", "or", "and", "timed-event", "timed-or", "timed-and", "cycle", "count")
)
def test_typed_reset_teardown(tmp_path, datatype, wait_kind):
    """Check multiword/wide native teardown after every guarded wait family."""
    assert "RESET_TEARDOWN_OK 6 4 ns" in run_example(tmp_path, "tests/models/reset_teardown.py", (wait_kind, datatype))


@pytest.mark.parametrize("operation", ("get", "master", "slave"))
def test_tlm_reset_teardown_native_reference(tmp_path, operation):
    """Compare native blocking TLM get/peek cleanup and deletion with Python paths."""
    output = run_example(tmp_path, "tests/models/reset_teardown.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_reset_teardown.cpp"),
            "--",
            operation,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    records = [
        line
        for line in output.splitlines()
        if line.startswith((f"TEARDOWN_REFERENCE {operation} ", "TEARDOWN_REFERENCE all "))
    ]
    assert len(records) == 9
    assert records == [line for line in native.stdout.splitlines() if line.startswith("TEARDOWN_REFERENCE")]
