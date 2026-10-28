"""Compare small public TLM interface families with native implementations."""

import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path):
    """Run the standalone native interface reference under a fresh kernel.

    Args:
        tmp_path: Isolated native build and simulation directory.

    Returns:
        Captured standard output from the native reference.

    Raises:
        AssertionError: If native compilation or simulation fails.
        subprocess.TimeoutExpired: If the reference exceeds sixty seconds.
    """
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_interfaces_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def interface_records(output):
    """Select the deterministic value and callback markers for comparison."""
    return [line for line in output.splitlines() if line.startswith("TLM_IFACE_")]


def test_tlm_public_interface_families_match_native(tmp_path):
    """Check wide/custom values, inheritance aliases and TLM-2 dispatch."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/tlm_interfaces_public.py")
    native_output = run_native(native_dir)
    actual = interface_records(python_output)
    expected = interface_records(native_output)
    assert actual == expected, (actual, expected)
    assert actual[0] == ("TLM_IFACE_WIDE 17 1 17 1 17 1 17 1 17 1 17 1 91 1 93 1 1 1 1 0 0 1 1 4 4")
    assert actual[1] == "TLM_IFACE_WRITE 37 42 1 1 3000"
    assert actual[2] == "TLM_IFACE_FIFO 2 5 21 20 1 1 17 1 23 1 31 1"
    assert actual[3] == "TLM_IFACE_PROTOCOL 3000 1 2 1 16 31 4 1 1 1 1 1 1"
    assert actual[4] == "TLM_IFACE_DESTRUCTION 1 1 1 1"
    assert "TLM_INTERFACES_PUBLIC_OK" in python_output
