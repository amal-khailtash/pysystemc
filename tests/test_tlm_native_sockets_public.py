"""Compare the public native TLM socket matrix with its C++ reference."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_native_sockets_match_cpp_reference(tmp_path):
    """Check every socket family, binding overload and interface result."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_output = run_example(python_dir, "tests/models/tlm_native_sockets_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_native_sockets_public.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("SOCKET_")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("SOCKET_")]
    assert len(python_records) == len(native_records) == 9
    assert python_records == native_records
    assert "TLM_NATIVE_SOCKETS_PYTHON_OK" in python_output
    assert "TLM_NATIVE_SOCKETS_NATIVE_OK" in native.stdout
