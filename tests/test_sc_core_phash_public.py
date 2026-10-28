"""Compare generic hash public declarations across native and custom parameter types."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_phash_public(tmp_path):
    """Verify all hash overloads, typed outputs, ownership and iterator signatures."""
    output = run_example(tmp_path, "tests/models/sc_core_phash_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_phash_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("PHASH_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("PHASH_")]
    assert actual == expected
    assert len(actual) == 12
    for label in ("custom", "length", "fixed", "cast"):
        assert f"PHASH_BORROWED {label} 0 0 1 1 0 1 3 1 1 1" in actual
        assert f"PHASH_CONTENTS {label} 2 1 0" in actual
        assert f"PHASH_OWNERSHIP {label} 8 8 0 0" in actual
