"""Compare clock and pointer-vector bindings with independent native models."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_clock_vector_public(tmp_path):
    """Check clock timing, reports, pointer-vector mutations, and callback order."""
    output = run_example(tmp_path, "tests/models/sc_core_clock_vector_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_clock_vector_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_reference = [line for line in output.splitlines() if line.startswith("SC_CORE_CLOCK_VECTOR ")]
    native_reference = [line for line in native.stdout.splitlines() if line.startswith("SC_CORE_CLOCK_VECTOR ")]
    assert len(python_reference) == len(native_reference) == 1
    assert python_reference == native_reference
