"""Compare small public enum and alias families with exact native consumers."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_small_types_public(tmp_path):
    """Verify nine enum types, action bits and six modeled module/port aliases."""
    output = run_example(tmp_path, "tests/models/sc_core_small_types_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_small_types_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("SMALL_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("SMALL_")]
    assert actual == expected
    assert len(actual) == 11
    assert actual[-1] == "SMALL_ALIAS aliases edges=0,2000,4000 queue=1000,3000 io=0 out=0 end=5000"
