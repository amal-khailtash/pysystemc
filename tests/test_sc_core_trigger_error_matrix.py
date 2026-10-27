"""Compare exact free next_trigger error metadata and count with native C++."""

import subprocess
import sys

from .support import ROOT, run_example


def test_next_trigger_error_matrix(tmp_path):
    """Require 36 single native E520 reports, recovery and untouched host state."""
    output = run_example(tmp_path, "tests/models/sc_core_trigger_error_matrix.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_trigger_error_matrix.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("TRIGGER_ERROR ")]
    assert len(actual) == 36
    assert actual == [line for line in native.stdout.splitlines() if line.startswith("TRIGGER_ERROR ")]
    assert "TRIGGER_ERROR_OK reports=36 invalid_arguments=2" in output
