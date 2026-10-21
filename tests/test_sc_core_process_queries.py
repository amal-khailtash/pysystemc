"""Compare public process queries against an independent native model."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_process_queries(tmp_path):
    """Verify current process/kind, last-handle fallback and callback states."""
    output = run_example(tmp_path, "tests/models/sc_core_process_queries.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_process_queries.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [
        line for line in output.splitlines() if line.startswith(("PROCESS_QUERY_REFERENCE", "PROCESS_UNWIND_REFERENCE"))
    ]
    expected = [
        line
        for line in native.stdout.splitlines()
        if line.startswith(("PROCESS_QUERY_REFERENCE", "PROCESS_UNWIND_REFERENCE"))
    ]
    assert len(actual) == 8
    assert actual == expected
