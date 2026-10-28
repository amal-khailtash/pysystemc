"""Compare public PEQ declarations with a separate native SystemC simulation."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_peq_public_matches_native_reference(tmp_path):
    """Check arbitrary payloads, queue aliases, callback constructors and timing."""
    python_output = run_example(tmp_path, "tests/models/tlm_peq_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/tlm_peq_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    python_get = [line for line in python_output.splitlines() if line.startswith("PEQ_GET ")]
    native_get = [line for line in native.stdout.splitlines() if line.startswith("PEQ_GET ")]
    python_callbacks = [line for line in python_output.splitlines() if line.startswith("PEQ_CB ")]
    native_callbacks = [line for line in native.stdout.splitlines() if line.startswith("PEQ_CB ")]
    python_default = [line for line in python_output.splitlines() if line.startswith("PEQ_DEFAULT ")]
    native_default = [line for line in native.stdout.splitlines() if line.startswith("PEQ_DEFAULT ")]
    assert python_get == native_get
    assert python_callbacks == native_callbacks
    assert python_default == native_default
    assert "PEQ_PAIR_TYPE_OK" in python_output
    assert "PEQ_CANCEL_OK 6 ns" in python_output
    assert "PEQ_CANCEL_OK 6 ns" in native.stdout
    assert "PEQ_PYTHON_OK" in python_output
    assert "PEQ_NATIVE_OK" in native.stdout
