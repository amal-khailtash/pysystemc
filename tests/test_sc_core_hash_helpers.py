"""Compare native hash helpers against an independent native model."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_hash_helpers(tmp_path):
    """Verify exact hashes, compare signs and duplicate/free ownership."""
    output = run_example(tmp_path, "tests/models/sc_core_hash_helpers.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_hash_helpers.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if "HASH" in line and "REFERENCE" in line]
    expected = [line for line in native.stdout.splitlines() if "HASH" in line and "REFERENCE" in line]
    assert len(actual) == 10
    assert actual == expected
