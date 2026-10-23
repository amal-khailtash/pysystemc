"""Compare small and partially completed native kernel families."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_small_gaps_public(tmp_path):
    """Verify live flags, native diagnostics, owned reports and event expression timing."""
    output = run_example(tmp_path, "tests/models/sc_core_small_gaps_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_small_gaps_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("SMALL_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("SMALL_")]
    assert len(actual) == 18 and actual == expected
