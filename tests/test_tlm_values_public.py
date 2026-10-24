"""Compare public DMI, phase and quantumkeeper values with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_values_public_match_native_reference(tmp_path):
    """Check exact DMI/phase values and quantum lifecycle timing in fresh processes."""
    python_output = run_example(tmp_path, "tests/models/tlm_values_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/tlm_values_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "TLM_VALUES_PUBLIC_OK" in python_output
    python_records = [line for line in python_output.splitlines() if line.startswith("TLM_VALUES_REFERENCE ")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("TLM_VALUES_REFERENCE ")]
    assert python_records == native_records
