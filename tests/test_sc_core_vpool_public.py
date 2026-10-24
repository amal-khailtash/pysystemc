"""Compare typed sc_vpool operations with the native SystemC reference."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_vpool_public(tmp_path):
    """Verify all pool specializations, wrap reuse, alignment and raw lifetimes."""
    output = run_example(tmp_path, "tests/models/sc_core_vpool_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_vpool_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("VPOOL")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("VPOOL")]
    assert len(actual) == 19 and actual == expected
