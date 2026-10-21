"""Compare time configuration contracts against an independent native model."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_time_configuration(tmp_path):
    """Verify configuration defaults, changes, native errors and time limits."""
    output = run_example(tmp_path, "tests/models/sc_core_time_configuration.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_time_configuration.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("TIME_CONFIGURATION_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("TIME_CONFIGURATION_REFERENCE")]
    assert len(actual) == 1
    assert actual == expected
