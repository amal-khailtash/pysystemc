"""Compare remaining recoverable kernel diagnostics with native SystemC."""

import os
import subprocess
import sys

import pytest

from .support import ROOT, run_example


SCENARIOS = {
    "orphan": (558,),
    "idle_start": (571,),
    "overflow": (544,),
    "unwinding": (540,),
    "self_notify": (536,),
    "module_warnings": (569, 569, 509, 509, 510),
    "module_stack": (511,),
    "after_error": (548,),
    "raw_controls": (572, 573),
    "nested_start": (554,),
    "cycle_misses": (538,),
    "phase_callback_forbidden": (553, 553, 553, 553),
    "module_binding_error": (107,),
}


def _semantic_rows(output):
    """Return report and final-state records from a subprocess transcript."""
    prefixes = ("REMAINING_DIAGNOSTIC_REPORT|", "REMAINING_DIAGNOSTIC_STATE|")
    return [line for line in output.splitlines() if line.startswith(prefixes)]


@pytest.mark.parametrize(("scenario", "expected_ids"), SCENARIOS.items())
def test_sc_core_remaining_diagnostics(tmp_path, scenario, expected_ids):
    """Match exact captured diagnostics and final state with a fresh native run."""
    output = run_example(tmp_path, "tests/models/sc_core_remaining_diagnostics.py", (scenario,))
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_remaining_diagnostics.cpp"),
            "--",
            scenario,
        ],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = _semantic_rows(output)
    expected = _semantic_rows(native.stdout)
    report_rows = [line for line in actual if line.startswith("REMAINING_DIAGNOSTIC_REPORT|")]
    assert tuple(int(line.split("|", 4)[3]) for line in report_rows) == expected_ids
    assert len([line for line in actual if line.startswith("REMAINING_DIAGNOSTIC_STATE|")]) == 1
    assert actual == expected
