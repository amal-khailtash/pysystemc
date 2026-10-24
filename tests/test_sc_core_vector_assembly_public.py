"""Compare public member-assembly behavior against an independent native model."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path):
    """Build and run the native assembly reference in an isolated process.

    Args:
        tmp_path: Working directory for native compiler and simulation output.

    Returns:
        Captured native standard output.

    Raises:
        AssertionError: If native compilation or simulation fails.
        subprocess.TimeoutExpired: If the process exceeds sixty seconds.
    """
    environment = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_vector_assembly_public.cpp"),
        ],
        cwd=tmp_path,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def test_sc_core_vector_assembly_public(tmp_path):
    """Check assembly aliases, all binding routes, native identity, time and teardown."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_output = run_example(python_dir, "tests/models/sc_core_vector_assembly_public.py")
    native_output = run_native(native_dir)
    prefix = "SC_CORE_VECTOR_ASSEMBLY "
    python_records = [line for line in python_output.splitlines() if line.startswith(prefix)]
    native_records = [line for line in native_output.splitlines() if line.startswith(prefix)]
    expected = "SC_CORE_VECTOR_ASSEMBLY count=10x2 aliases=1 bindings=10 time=1ns empty_report=807,807,807 destroyed=20"
    assert python_records == native_records == [expected]
