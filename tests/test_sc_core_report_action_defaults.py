"""Compare defaulted report-action calls with a fresh native SystemC oracle."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_report_action_defaults(tmp_path):
    """Compare prior values, reset values and resolved fallback actions."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    output = run_example(python_dir, "tests/models/sc_core_report_action_defaults.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_report_action_defaults.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    actual = [line for line in output.splitlines() if line.startswith("REPORT_ACTION_DEFAULT ")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("REPORT_ACTION_DEFAULT ")]
    assert len(actual) == len(expected) == 3
    assert actual == expected
    assert "REPORT_ACTION_DEFAULT_CASES 3" in output
