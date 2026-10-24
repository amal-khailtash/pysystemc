"""Compare public sc_vector bindings with native SystemC behavior."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def _run_native(tmp_path):
    """Compile and run the native vector reference in an isolated process.

    Args:
        tmp_path: Fresh working directory for native build products.

    Returns:
        Captured native standard output.

    Raises:
        AssertionError: If compilation or native simulation fails.
        subprocess.TimeoutExpired: If native work exceeds sixty seconds.
    """
    environment = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_vector_container_public.cpp"),
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
    """Select the stable semantic record from SystemC's startup output."""
    records = [line for line in output.splitlines() if line.startswith("SC_CORE_VECTOR_CONTAINER ")]
    assert len(records) == 1, output
    return records[0]


def test_sc_core_vector_container_public(tmp_path):
    """Check constructors, custom creation, bindings, base APIs, and ownership."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/sc_core_vector_container_public.py")
    native_output = _run_native(native_dir)
    expected = (
        "SC_CORE_VECTOR_CONTAINER sizes=2,2,3,3,3 kind=sc_vector elements=3 creator=10,21,32 "
        "const=10,21 iter=3,3 signal=0,7,14 binds=2,1,2,6,4,1,5,5,5,2,4 "
        "reports=805,807,807 lifecycle=1,2 aliases=1"
    )
    assert _record(python_output) == _record(native_output) == expected
