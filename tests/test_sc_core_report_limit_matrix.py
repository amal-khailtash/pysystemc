"""Compare Python stop-after limits and malformed calls with native behavior."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_report_limit_matrix(tmp_path):
    """Verify threshold boundaries and rejected calls in fresh SystemC processes."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    output = run_example(python_dir, "tests/models/sc_core_report_limit_matrix.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_report_limit_matrix.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    actual = [line for line in output.splitlines() if line.startswith("REPORT_LIMIT ")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("REPORT_LIMIT ")]
    assert len(actual) == len(expected) == 45
    assert actual == expected

    actual_precedence = [line for line in output.splitlines() if line.startswith("REPORT_PRECEDENCE ")]
    expected_precedence = [line for line in native.stdout.splitlines() if line.startswith("REPORT_PRECEDENCE ")]
    assert len(actual_precedence) == len(expected_precedence) == 6
    assert actual_precedence == expected_precedence

    rejected = [line for line in output.splitlines() if line.startswith("REPORT_REJECT ")]
    assert len(rejected) == 11
    assert {line.split()[1] for line in rejected if "overflow" in line} == {
        "severity_overflow",
        "message_overflow",
        "message_severity_overflow",
    }
