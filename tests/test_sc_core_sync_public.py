"""Check remaining mutex/semaphore family contracts against native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_sync_public(tmp_path):
    """Compare protected probes, interface constructors and teardown results."""
    output = run_example(tmp_path, "tests/models/sc_core_sync_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_sync_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("SYNC_PUBLIC_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("SYNC_PUBLIC_REFERENCE")]
    labels = ["mutex_interface", "semaphore_interface", "report_error", "in_use", "destruction"]
    assert [line.split("\t")[1] for line in actual] == labels
    assert [line.split("\t")[1] for line in expected] == labels
    assert actual == expected
