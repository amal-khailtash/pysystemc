"""Validate fatal report actions without aborting the pytest process."""

import subprocess
import sys

import pytest

from .support import ROOT


@pytest.mark.parametrize("mode", ("throw", "abort", "assert_abort"))
def test_sc_core_fatal_public(tmp_path, mode):
    """Compare report fields or native abnormal termination in fresh subprocesses."""
    python = subprocess.run(
        [sys.executable, str(ROOT / "tests/models/sc_core_fatal_public.py"), mode],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_fatal_public.cpp"),
            "--",
            mode,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    if mode == "throw":
        assert python.returncode == native.returncode == 0, python.stderr + native.stderr
        actual = [line for line in python.stdout.splitlines() if line.startswith("FATAL_THROW")]
        expected = [line for line in native.stdout.splitlines() if line.startswith("FATAL_THROW")]
        assert len(actual) == 1 and actual == expected
    else:
        assert python.returncode == native.returncode != 0, python.stderr + native.stderr
        assert f"FATAL_ENTER {mode}" in python.stdout and f"FATAL_ENTER {mode}" in native.stdout
        diagnostic = "simulation aborted" if mode == "abort" else "assertion failed"
        assert diagnostic in python.stdout + python.stderr
        assert diagnostic in native.stdout + native.stderr
