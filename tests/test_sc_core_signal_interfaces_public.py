"""Compare generic, bool and logic signal-interface contracts with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_signal_interfaces_public(tmp_path):
    """Verify borrowed reference identity, typed virtual dispatch, edges and destruction."""
    output = run_example(tmp_path, "tests/models/sc_core_signal_interfaces_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_signal_interfaces_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("SIGNAL_INTERFACE_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("SIGNAL_INTERFACE_")]
    assert len(actual) == 5 and actual == expected
