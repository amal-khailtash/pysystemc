"""Acceptance test for unchanged native SystemC report presentation."""

import subprocess
import sys

from .support import ROOT


def selected_records(output):
    """Select byte-encoded native report observations from process output.

    Args:
        output: Captured stdout from one isolated model process.

    Returns:
        The ordered warning and SC_THROW records used for native comparison.
    """
    return [line for line in output.splitlines() if line.startswith(("PRES_REPORT\t", "PRES_THROW\t"))]


def test_native_reports_survive_presentation_and_adapter_errors(tmp_path):
    """Compare Python presentation boundaries with native report behavior.

    Python and C++ each run in a fresh subprocess because the native report
    handler and SystemC kernel have process-local singleton state.
    """
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python = subprocess.run(
        [sys.executable, str(ROOT / "tests/models/presentation_report_preservation.py")],
        cwd=python_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert python.returncode == 0, python.stdout + python.stderr
    python_output = python.stdout
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/presentation_report_preservation.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    actual = selected_records(python_output)
    expected = selected_records(native.stdout)
    assert actual == expected
    assert len(actual) == 5
    assert "PRESENTATION_REPORT_PRESERVATION_OK" in python_output
