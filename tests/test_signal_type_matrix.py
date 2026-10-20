"""Compare structural scalar signal types and errors with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_signal_type_matrix_matches_native_reference(tmp_path):
    """Compare scalar endpoint/special transitions and callback recovery."""
    output = run_example(tmp_path, "tests/models/signal_type_matrix.py")
    assert "SIGNAL_CALLBACK_ERRORS_OK" in output
    assert "SIGNAL_TYPE_MATRIX_OK" in output
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/signal_type_matrix.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = ("SIGNAL_MATRIX ", "SIGNAL_FLOAT ")
    python_records = [line for line in output.splitlines() if line.startswith(prefixes)]
    native_records = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert python_records == native_records
    assert len(python_records) == 8
