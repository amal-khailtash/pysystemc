"""Compare small public sc_dt values and scalar aliases with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_dt_values_public_match_native_reference(tmp_path):
    """Verify enum values, native format strings, constant identity and numeric limits."""
    python_output = run_example(tmp_path, "tests/models/sc_dt_values_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_values_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("SC_DT_")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("SC_DT_")]
    assert python_records == native_records
    assert len(python_records) == 122
