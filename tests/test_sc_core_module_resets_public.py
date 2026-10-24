"""Compare Python sc_module reset overloads with native SystemC callbacks."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_module_reset_signal_overloads_match_native(tmp_path):
    """Verify all eight exact reset signatures and cleanup timestamps natively."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/sc_core_module_resets_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_module_resets_public.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    actual = [line for line in python_output.splitlines() if line.startswith("MODULE_RESET|")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("MODULE_RESET|")]
    assert len(actual) == len(expected) == 8
    assert actual == expected
    signatures = [tuple(line.split("|", 5)[:5]) for line in actual]
    assert signatures == [
        ("MODULE_RESET", "sync", "sc_in", "reset_signal_is", "const sc_core::sc_in<bool>&,bool"),
        ("MODULE_RESET", "sync", "sc_inout", "reset_signal_is", "const sc_core::sc_inout<bool>&,bool"),
        ("MODULE_RESET", "sync", "sc_out", "reset_signal_is", "const sc_core::sc_out<bool>&,bool"),
        (
            "MODULE_RESET",
            "sync",
            "sc_signal_in_if",
            "reset_signal_is",
            "const sc_core::sc_signal_in_if<bool>&,bool",
        ),
        ("MODULE_RESET", "async", "sc_in", "async_reset_signal_is", "const sc_core::sc_in<bool>&,bool"),
        (
            "MODULE_RESET",
            "async",
            "sc_inout",
            "async_reset_signal_is",
            "const sc_core::sc_inout<bool>&,bool",
        ),
        ("MODULE_RESET", "async", "sc_out", "async_reset_signal_is", "const sc_core::sc_out<bool>&,bool"),
        (
            "MODULE_RESET",
            "async",
            "sc_signal_in_if",
            "async_reset_signal_is",
            "const sc_core::sc_signal_in_if<bool>&,bool",
        ),
    ]
    assert all("starts=0," in line and "|cleanups=" in line for line in actual)
