"""Compare callable bindings and reset cleanup against native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_bind_public(tmp_path):
    """Verify native copying, borrowed references, placeholders and bound resets."""
    actual = run_example(tmp_path, "tests/models/sc_core_bind_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_bind_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    records = [line for line in actual.splitlines() if line.startswith("BIND_")]
    assert records == [line for line in native.stdout.splitlines() if line.startswith("BIND_")]
    assert len(records) == 3
