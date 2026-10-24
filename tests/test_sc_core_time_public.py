"""Check all time-family declarations against independent native values."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_time_public(tmp_path):
    """Verify exact constructors, conversions, mutation identities and diagnostics."""
    output = run_example(tmp_path, "tests/models/sc_core_time_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_time_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("TIME_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("TIME_")]
    assert actual == expected
    assert len(actual) == 31
    assert actual[-1] == "TIME_RECOVERY 3000"
    assert "TIME_RELATIONS 1 1 1 1 1 1" in actual
    assert sum(line.startswith("TIME_ERROR ") for line in actual) == 9
