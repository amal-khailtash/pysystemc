"""Compare public hash-base behavior with an independent native reference."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_hash_base_public(tmp_path):
    """Verify constructors, callbacks, copies, iterator mutation and ownership."""
    expected = (
        "SC_HASH_BASE_REFERENCE rows=45 counts=0,3,2 copies=2,2 iterator=3 removed=6 freed=12 bins=5,11 elem=null"
    )
    output = run_example(tmp_path, "tests/models/sc_core_hash_base_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_hash_base_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert expected in output
    assert expected in native.stdout
