"""Check semantic TLM FIFO adapter edges in isolated simulations."""

import subprocess
import sys

from .support import ROOT, run_example


def test_fifo_error_recovery_matches_native_reference(tmp_path):
    """Check stable adaptation and state-preserving malformed reads."""
    python_output = run_example(tmp_path, "tests/models/tlm_adapter_error_edges.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_adapter_error_edges.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_record = next(line for line in python_output.splitlines() if line.startswith("TLM_ADAPTER_FIFO "))
    native_record = next(line for line in native.stdout.splitlines() if line.startswith("TLM_ADAPTER_FIFO "))
    assert python_record == native_record
