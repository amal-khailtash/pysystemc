"""Compare direct Python passthrough B interfaces with a native reference."""

import subprocess
import sys

from .support import ROOT, run_example


def test_passthrough_b_interfaces_match_native_reference(tmp_path):
    """Verify both untagged and tagged native target B callback interfaces."""
    python_output = run_example(tmp_path, "tests/models/tlm_utils_passthrough_b_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_utils_passthrough_b_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("B_SOCKET_EVENT ")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("B_SOCKET_EVENT ")]
    assert python_records == native_records
    assert "B_SOCKET_LIFETIME 1" in python_output
    assert "B_SOCKET_LIFETIME 1" in native.stdout
    assert "TLM_UTILS_PASSTHROUGH_B_PUBLIC_OK" in python_output
    assert "TLM_UTILS_PASSTHROUGH_B_PUBLIC_OK" in native.stdout
