"""Check native blocking synchronization cleanup through borrowed interfaces."""

import subprocess
import sys

import pytest
from .support import ROOT, run_example


@pytest.mark.parametrize("kind", ("mutex", "semaphore"))
@pytest.mark.parametrize("access", ("direct", "port"))
def test_native_sync_guarded_reset(tmp_path, kind, access):
    """Assert reset/kill timing, Python cleanup and native resource retention."""
    output = run_example(tmp_path, "tests/models/native_sync_guarded_reset.py", (kind, access))
    assert "NATIVE_SYNC_GUARDED_RESET_OK" in output
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sync_guarded_reset.cpp"),
            "--",
            kind,
            access,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert [line for line in output.splitlines() if line.startswith("SYNC_RESET_REFERENCE")] == [
        line for line in native.stdout.splitlines() if line.startswith("SYNC_RESET_REFERENCE")
    ]
