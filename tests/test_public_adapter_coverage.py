"""Compare public report assignment and resolved reads with native SystemC."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path):
    """Compile and run the isolated native adapter reference with a timeout.

    Args:
        tmp_path: Fresh directory for native build products.

    Returns:
        Captured standard output from the native reference process.

    Raises:
        AssertionError: Native compilation or simulation fails.
        subprocess.TimeoutExpired: The native process exceeds sixty seconds.
    """
    environment = dict(os.environ)
    environment.setdefault("SYSTEMC_HOME", str(ROOT / "submodules/systemc"))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/public_adapter_coverage.cpp"),
        ],
        cwd=tmp_path,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def selected_records(output):
    """Select deterministic records emitted by both Python and native cases."""
    prefixes = ("REPORT_ASSIGN ", "REPORT_SELF ", "RESOLVED_READ ", "RESOLVED_FINAL ")
    return [line for line in output.splitlines() if line.startswith(prefixes)]


def test_public_report_assignment_and_resolved_reads_match_native(tmp_path):
    """Check copied report fields, assignment identity, and resolved read timing."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/public_adapter_coverage.py")
    native_output = run_native(native_dir)

    actual = selected_records(python_output)
    expected = selected_records(native_output)
    assert actual == expected
    report = next(line for line in actual if line.startswith("REPORT_ASSIGN "))
    assert report.split()[1:9] == [
        "1",  # assignment returned the destination object
        "-1",  # report ID for an unregistered message type
        str(1),  # SC_WARNING
        "assignment/source",
        "copied",
        "source.cpp",
        "73",
        "0",  # report timestamp in this pre-simulation check
    ]
    assert "REPORT_SELF 1 1" in actual
    assert [line.split()[1:] for line in actual if line.startswith("RESOLVED_READ ")] == [
        ["1", "ns", "0"],
        ["2", "ns", "X"],
        ["3", "ns", "1"],
        ["4", "ns", "0"],
    ]
    assert "RESOLVED_FINAL 5 ns 0" in actual
