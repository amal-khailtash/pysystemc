"""Compare typed native hash families with the Python-facing cppyy model."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_hash_typed_public(tmp_path):
    """Verify typed operations, deep key copies, iterator changes and key lifetimes."""
    output = run_example(tmp_path, "tests/models/sc_core_hash_typed_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_hash_typed_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("HASH_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("HASH_")]
    assert len(actual) == 15 and actual == expected
