"""Compare public TLM utility socket declarations and native dispatch behavior."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_socket_wrappers_match_native_reference(tmp_path):
    """Check all wrapper constructors plus custom-protocol binding and transport."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_output = run_example(python_dir, "tests/models/tlm_socket_wrappers_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_socket_wrappers_public.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("SOCKET_WRAPPER_")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("SOCKET_WRAPPER_")]
    assert python_records == native_records
    assert "SOCKET_WRAPPERS_PYTHON_OK" in python_output
    assert "SOCKET_WRAPPERS_NATIVE_OK" in native.stdout
