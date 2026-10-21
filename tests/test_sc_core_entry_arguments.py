"""Compare direct public entry arguments with native mutation isolation."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_entry_arguments(tmp_path):
    """Verify UTF-8/empty arguments and original argv copied-return behavior."""
    arguments = ("café", "--flag", "")
    output = run_example(tmp_path, "tests/models/sc_core_entry_arguments.py", arguments)
    native = subprocess.run(
        [
            sys.executable,
            "-m",
            "amal.eda.systemc._runner",
            str(ROOT / "tests/native/sc_core_entry_arguments.cpp"),
            "--",
            *arguments,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("ENTRY_ARGUMENT_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("ENTRY_ARGUMENT_REFERENCE")]
    assert len(actual) == 1
    assert actual == expected
