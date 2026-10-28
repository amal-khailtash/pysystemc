"""Compare Python vector access policies with a fresh native reference run."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_vector_access_public(tmp_path):
    """Check policy conversions, member identity, iterator algebra and ownership."""
    output = run_example(tmp_path, "tests/models/sc_core_vector_access_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_vector_access_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("VECTOR_ACCESS_REFERENCE ")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("VECTOR_ACCESS_REFERENCE ")]
    assert len(actual) == len(expected) == 1
    assert actual == expected
