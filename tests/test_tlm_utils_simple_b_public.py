"""Compare four public simple socket `_b` families with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_utils_simple_b_public_matches_native(tmp_path):
    """Check aliases, registrations, DMI and bidirectional transport results."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_output = run_example(python_dir, "tests/models/tlm_utils_simple_b_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_utils_simple_b_public.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("SIMPLE_B_REC|")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("SIMPLE_B_REC|")]
    assert len(python_records) == len(native_records) == 24
    assert python_records == native_records
    assert "SIMPLE_B_NATIVE_OWNER_OK" in python_output
    assert "SIMPLE_B_NATIVE_REFERENCE_OK" in native.stdout
