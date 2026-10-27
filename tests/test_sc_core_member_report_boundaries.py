"""Compare invalid module/channel scheduler members and W547 with native runs."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


SCENARIOS = ("module", "channel", "stop_mode")
EXPECTED_REPORTS = {"module": 14, "channel": 12, "stop_mode": 1}


@pytest.mark.parametrize("scenario", SCENARIOS)
def test_sc_core_member_report_boundaries(tmp_path, scenario):
    """Require exact recoverable report metadata, counts, and final kernel state."""
    output = run_example(
        tmp_path,
        "tests/models/sc_core_member_report_boundaries.py",
        (scenario,),
    )
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_member_report_boundaries.cpp"),
            "--",
            scenario,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = (
        "MEMBER_BOUNDARY_REPORT ",
        "MEMBER_BOUNDARY_RECOVERED ",
        "MEMBER_BOUNDARY_FINAL ",
        "MEMBER_BOUNDARY_OK ",
    )
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert sum(line.startswith("MEMBER_BOUNDARY_REPORT ") for line in actual) == EXPECTED_REPORTS[scenario]
    assert sum(line.startswith("MEMBER_BOUNDARY_RECOVERED ") for line in actual) == EXPECTED_REPORTS[scenario]
    assert f"MEMBER_BOUNDARY_OK {scenario} reports={EXPECTED_REPORTS[scenario]}" in actual
    assert actual == expected
