"""Compare process utilities and free handle predicates with native SystemC."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("mode", ("normal", "error"))
def test_sc_core_process_utilities_public(tmp_path, mode):
    """Verify native halt cleanup, stack size, handle comparisons and invalid context."""
    arguments = () if mode == "normal" else ("error",)
    output = run_example(tmp_path, "tests/models/sc_core_process_utilities_public.py", arguments)
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_process_utilities_public.cpp"),
            "--",
            *arguments,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = ("HALT_RECORD", "HALT_ERROR", "PROCESS_COMPARE", "PROCESS_FINAL", "STACK_CONFIG")
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert actual == expected
    assert len(actual) == (13 if mode == "normal" else 1)
