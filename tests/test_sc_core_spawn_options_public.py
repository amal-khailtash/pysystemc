"""Compare public spawn-option overloads and lifecycle against native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_spawn_options_public(tmp_path):
    """Verify every option family and compare callback/reset timelines with C++."""
    output = run_example(tmp_path, "tests/models/sc_core_spawn_options_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_spawn_options_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    markers = ("OPTION_DESTRUCTOR", "OPTION_CALLBACK", "OPTION_RESET", "OPTION_FINAL")
    actual = [line for line in output.splitlines() if line.startswith(markers)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(markers)]
    assert len(actual) == 31, actual
    assert actual == expected, (actual, expected)
