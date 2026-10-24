"""Compare guarded free wait and dynamic next_trigger overload families."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_wait_next_public(tmp_path):
    """Match all wait/trigger overload callbacks, times, deltas and restoration."""
    output = run_example(tmp_path, "tests/models/sc_core_wait_next_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_wait_next_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = (
        "WAIT_NEXT_WAIT_REFERENCE",
        "WAIT_NEXT_TRIGGER_REFERENCE",
        "WAIT_NEXT_CLEANUP_REFERENCE",
        "WAIT_NEXT_ERROR_REFERENCE",
    )
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert len(actual) == 5 and actual == expected
