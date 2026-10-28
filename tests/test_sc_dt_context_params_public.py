"""Compare public fixed-point parameter and context APIs with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_dt_context_parameter_matrix_matches_native(tmp_path):
    """Compare constructor, value, formatting and local context lifecycle records."""
    python_output = run_example(tmp_path, "tests/models/sc_dt_context_params_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_context_params_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = ("FX ", "LENGTH ", "CAST ", "CTX ")
    python_records = [line for line in python_output.splitlines() if line.startswith(prefixes)]
    native_records = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert python_records == native_records
    assert "SC_DT_CONTEXT_PARAMS_PUBLIC_OK" in python_output
    assert "SC_DT_CONTEXT_PARAMS_PUBLIC_OK" in native.stdout
