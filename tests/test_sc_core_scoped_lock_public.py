"""Compare guarded Python scoped locks with native scheduler behavior."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_scoped_lock_public(tmp_path):
    """Verify blocking acquisition, idempotent release, destruction and unwind."""
    output = run_example(tmp_path, "tests/models/sc_core_scoped_lock_public.py")
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_scoped_lock_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    actual = [line for line in output.splitlines() if line.startswith("SCOPED_")]
    expected = [line for line in result.stdout.splitlines() if line.startswith("SCOPED_")]
    assert len(actual) == 4 and actual == expected
