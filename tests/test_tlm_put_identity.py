"""Check structural TLM put binding order and native identity."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_put_binding_identity(tmp_path):
    """Assert shared adapters/events, copied samples and exactly timed observers."""
    assert "TLM_PUT_IDENTITY 9 2 1 ns" in run_example(tmp_path, "tests/models/tlm_put_identity.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/tlm_put_identity.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "TLM_PUT_NATIVE 3 2 1 ns" in native.stdout
