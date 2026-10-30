"""Verify selected fixed-point constructor adapters against native SystemC."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path):
    """Compile and run the native constructor reference with a fresh kernel.

    Args:
        tmp_path: Isolated working directory for native compilation and output.

    Returns:
        Captured standard output from the native reference.

    Raises:
        AssertionError: Native compilation or simulation returns a failure code.
        subprocess.TimeoutExpired: Native execution exceeds sixty seconds.
    """
    environment = dict(os.environ)
    environment.setdefault("SYSTEMC_HOME", str(ROOT / "submodules/systemc"))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/fixed_constructor_verification.cpp"),
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
    """Select stable value and rejection evidence, excluding kernel banners."""
    return [line for line in output.splitlines() if line.startswith("FCV\t")]


def test_fixed_constructor_contracts_match_native(tmp_path):
    """Compare fixed-template, configurable, and owned-number constructors."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/fixed_constructor_verification.py")
    native_output = run_native(native_dir)
    python_records = selected_records(python_output)
    native_records = selected_records(native_output)

    actual_values = [line for line in python_records if "rejected_" not in line]
    reference_values = [line for line in native_records if "rejected_" not in line]
    assert actual_values == reference_values
    assert len(actual_values) == 8
    assert "FCV\trejected_fx_config\t1\t1" in python_records
    assert "FCV\trejected_owner_default\t1" in python_records
    assert "FCV\trejected_owner_source\t1" in python_records
    assert "FCV\towner_slow_snapshot\t" in actual_values[5]
    assert "FCV\towner_fast_snapshot\t" in actual_values[7]
