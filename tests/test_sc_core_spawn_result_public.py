"""Compare native typed dynamic-process assignment, errors and unwind to Python."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("scenario", ("normal", "error"))
def test_sc_core_spawn_result_public(tmp_path, scenario):
    """Verify scalar/custom/137-bit results, method updates and reset/kill cleanup."""
    arguments = () if scenario == "normal" else (scenario,)
    output = run_example(tmp_path, "tests/models/sc_core_spawn_result_public.py", arguments)
    command = [
        sys.executable,
        str(ROOT / "examples/run_sysc_cpp.py"),
        str(ROOT / "tests/native/sc_core_spawn_result_public.cpp"),
    ]
    if arguments:
        command.extend(("--", *arguments))
    native = subprocess.run(command, cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("SPAWN_RESULT")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("SPAWN_RESULT")]
    assert actual == expected
    assert actual and len(actual) == (4 if scenario == "normal" else 1)
