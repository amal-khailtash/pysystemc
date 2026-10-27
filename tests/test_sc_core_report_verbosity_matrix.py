"""Verify accepted verbosity boundaries against independent native reports."""

import subprocess
import sys

from .support import ROOT, run_example


def test_report_verbosity_boundaries(tmp_path):
    """Compare 360 cases including signed-int extremes and unfiltered severities."""
    output = run_example(tmp_path, "tests/models/sc_core_report_verbosity_matrix.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_report_verbosity_matrix.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("REPORT_VERBOSITY ")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("REPORT_VERBOSITY ")]
    assert len(actual) == len(expected) == 360
    assert actual == expected
    assert "REPORT_VERBOSITY_OK cases=360 restored=1" in output
