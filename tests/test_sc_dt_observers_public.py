"""Compare fixed-point observer dispatch and generic bases to native C++."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_dt_observers_and_generic_bases_match_native(tmp_path):
    """Verify callback order/values, factory pointers and virtual destruction."""
    actual = run_example(tmp_path, "tests/models/sc_dt_observers_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_dt_observers_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in actual.splitlines() if line.startswith("DT_")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("DT_")]
    assert len(python_records) == len(native_records) == 10
    assert python_records == native_records
    assert "SC_DT_OBSERVERS_PUBLIC_OK" in actual
    assert "SC_DT_OBSERVERS_PUBLIC_OK" in native.stdout
