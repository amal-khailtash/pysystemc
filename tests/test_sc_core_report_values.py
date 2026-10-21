"""Compare native report value semantics against an independent native model."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_report_values(tmp_path):
    """Verify report copies/getters/swap and composed messages."""
    output = run_example(tmp_path, "tests/models/sc_core_report_values.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_report_values.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [
        line for line in output.splitlines() if line.startswith(("REPORT_VALUE_REFERENCE", "REPORT_COPY_REFERENCE"))
    ]
    expected = [
        line
        for line in native.stdout.splitlines()
        if line.startswith(("REPORT_VALUE_REFERENCE", "REPORT_COPY_REFERENCE"))
    ]
    assert len(actual) == 4
    assert actual == expected
