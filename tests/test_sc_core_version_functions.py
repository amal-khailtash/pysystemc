"""Compare version string functions against an independent native model."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_version_functions(tmp_path):
    """Verify exact native static-string returns across repeated calls."""
    output = run_example(tmp_path, "tests/models/sc_core_version_functions.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_version_functions.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("VERSION_FUNCTION_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("VERSION_FUNCTION_REFERENCE")]
    assert len(actual) == 3
    assert actual == expected
