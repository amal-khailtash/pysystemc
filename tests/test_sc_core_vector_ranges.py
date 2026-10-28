"""Compare Python sc_vector assembly range behavior with a native reference."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def _run_native(tmp_path):
    """Build and run the native range reference in an isolated process.

    Args:
        tmp_path: Fresh working directory for native build products.

    Returns:
        Captured native standard output.

    Raises:
        AssertionError: If native compilation or simulation fails.
        subprocess.TimeoutExpired: If native work exceeds sixty seconds.
    """
    environment = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_vector_ranges.cpp"),
        ],
        cwd=tmp_path,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def _record(output):
    """Select the stable range contract record from simulator output."""
    records = [line for line in output.splitlines() if line.startswith("SC_CORE_VECTOR_RANGES ")]
    assert len(records) == 1, output
    return records[0]


def test_sc_core_vector_ranges(tmp_path):
    """Check bind and operator() stopping positions, source identities and values."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/sc_core_vector_ranges.py")
    native_output = _run_native(native_dir)
    expected = "SC_CORE_VECTOR_RANGES returns=2,2,4,4 bound=2,2,3,2 time=1ns"
    assert _record(python_output) == _record(native_output) == expected
