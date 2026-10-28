"""Compare arbitrary analysis FIFO payload behavior with the native model."""

import subprocess
import sys

from .support import ROOT, run_example


def test_arbitrary_analysis_fifo_values_match_native_reference(tmp_path):
    """Compare pointer identity, borrowed storage, and copied wide values."""
    python_output = run_example(tmp_path, "tests/models/tlm_analysis_values_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_analysis_values_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = ("ANALYSIS_VALUES_POINTER ", "ANALYSIS_VALUES_WIDE ")
    python_records = [line for line in python_output.splitlines() if line.startswith(prefixes)]
    native_records = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert python_records == native_records
    assert "TLM_ANALYSIS_VALUES_PUBLIC_OK" in python_output
    assert "TLM_ANALYSIS_VALUES_PUBLIC_OK" in native.stdout
