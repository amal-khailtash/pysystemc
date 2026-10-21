"""Compare report-handler family contracts against an independent native model."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_report_handler(tmp_path):
    """Verify handler overloads, policies, count/limit, caching, logs and release."""
    output = run_example(tmp_path, "tests/models/sc_core_report_handler.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_report_handler.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith(("HANDLER_REFERENCE", "HANDLER_FINAL_REFERENCE"))]
    expected = [
        line for line in native.stdout.splitlines() if line.startswith(("HANDLER_REFERENCE", "HANDLER_FINAL_REFERENCE"))
    ]
    assert len(actual) == 12
    assert actual == expected
