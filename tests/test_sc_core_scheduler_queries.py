"""Compare scheduler query states against an independent native model."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_scheduler_queries(tmp_path):
    """Verify timed pending activity, delta counts and lifecycle boundaries."""
    output = run_example(tmp_path, "tests/models/sc_core_scheduler_queries.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_scheduler_queries.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("SCHEDULER_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("SCHEDULER_REFERENCE")]
    assert len(actual) == 7
    assert actual == expected
