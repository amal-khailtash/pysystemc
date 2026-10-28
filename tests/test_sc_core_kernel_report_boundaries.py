"""Compare selected recoverable kernel reports with isolated native runs."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


SCENARIOS = ("wait_n", "empty_lists", "stop_start")
EXPECTED_REPORTS = {"wait_n": 2, "empty_lists": 4, "stop_start": 2}


@pytest.mark.parametrize("scenario", SCENARIOS)
def test_sc_core_kernel_report_boundaries(tmp_path, scenario):
    """Match wrapper/raw stop boundaries and report recovery in fresh processes."""
    output = run_example(tmp_path, "tests/models/sc_core_kernel_report_boundaries.py", (scenario,))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_kernel_report_boundaries.cpp"),
            "--",
            scenario,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = ("KERNEL_BOUNDARY_REPORT ", "KERNEL_BOUNDARY_RECOVERED ", "KERNEL_BOUNDARY_OK ")
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    report_prefix = "KERNEL_BOUNDARY_REPORT "
    assert sum(line.startswith(report_prefix) for line in actual) == EXPECTED_REPORTS[scenario]
    recovered_prefix = "KERNEL_BOUNDARY_RECOVERED "
    assert sum(line.startswith(recovered_prefix) for line in actual) == EXPECTED_REPORTS[scenario]
    assert f"KERNEL_BOUNDARY_OK {scenario} reports={EXPECTED_REPORTS[scenario]}" in actual
    assert f"KERNEL_BOUNDARY_OK {scenario} reports={EXPECTED_REPORTS[scenario]}" in expected
    assert actual == expected
