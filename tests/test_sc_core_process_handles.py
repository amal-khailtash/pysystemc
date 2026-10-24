"""Compare process-handle contracts against independent native SystemC."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_process_handles(tmp_path):
    """Assert exact metadata, reference ownership and controlled invocation times."""
    output = run_example(tmp_path, "tests/models/sc_core_process_handles.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_process_handles.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("HANDLE_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("HANDLE_REFERENCE")]
    assert len(actual) == 10 and actual == expected
