"""Compare generic SystemC ports and inventoried specializations with C++."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path):
    """Build and run the independent native sc_port reference with a timeout.

    Args:
        tmp_path: Isolated directory for the native build and execution.

    Returns:
        The native reference program's standard output.

    Raises:
        AssertionError: If compilation or native execution fails.
        subprocess.TimeoutExpired: If the native program exceeds sixty seconds.
    """
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_ports_public.cpp"),
        ],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def test_sc_port_and_sc_port_b_public_families(tmp_path):
    """Verify generic and inventoried ports, native accessors, and teardown."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/sc_core_ports_public.py")
    native_output = run_native(native_dir)
    marker = "SC_PORT_FAMILY_REFERENCE "
    python_lines = [line for line in python_output.splitlines() if line.startswith(marker)]
    native_lines = [line for line in native_output.splitlines() if line.startswith(marker)]
    assert len(python_lines) == 1, python_output
    assert python_lines == native_lines, (python_lines, native_lines)
