"""Compare selected deprecated SystemC API reports with native references."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


SCENARIOS = {
    "simcontext": (
        "find_object",
        "get_child_objects",
        "delta_count",
        "is_running",
        "initialize",
        "cycle",
        "simulation_time",
    ),
    "cthread": ("current_process_handle", "cthread_general_wait", "cthread_event_list_wait", "timed_out"),
    "module": ("positional_shift", "repeat_second"),
    "random_seed": ("random_seed",),
}


@pytest.mark.parametrize("scenario", SCENARIOS)
def test_sc_core_deprecation_reports(tmp_path, scenario):
    """Require exact report source, metadata, count, order, and final kernel state."""
    output = run_example(tmp_path, "tests/models/sc_core_deprecation_reports.py", (scenario,))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_deprecation_reports.cpp"),
            "--",
            scenario,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = ("DEPRECATION_REPORT ", "DEPRECATION_FINAL ", "DEPRECATION_OK ")
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    report_lines = [line for line in actual if line.startswith("DEPRECATION_REPORT ")]
    assert len(report_lines) == len(SCENARIOS[scenario])
    assert [line.split("|", 1)[0].split(" ", 1)[1] for line in report_lines] == list(SCENARIOS[scenario])
    assert f"DEPRECATION_OK {scenario} reports={len(SCENARIOS[scenario])}" in actual
    assert actual == expected
