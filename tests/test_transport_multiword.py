"""Verify C1 transport multiword value/output overload and callback error semantics."""

import subprocess
import sys

from .support import ROOT, run_example


def test_transport_multiword_overloads(tmp_path):
    """Check exact endpoints, wide bits, preserved caller output and recovery."""
    assert "TRANSPORT_MULTIWORD_OK 4 1 ns" in run_example(tmp_path, "tests/models/transport_multiword.py")
    result = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/transport_multiword.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "NATIVE_TRANSPORT_MULTIWORD_OK 4" in result.stdout
