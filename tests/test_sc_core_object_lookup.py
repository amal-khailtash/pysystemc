"""Compare public object/event/context lookups with native hierarchy semantics."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_object_lookup(tmp_path):
    """Verify present/null lookup results, borrowed identities and parent context."""
    output = run_example(tmp_path, "tests/models/sc_core_object_lookup.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_object_lookup.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "OBJECT_LOOKUP_REFERENCE 1 1 1 1 1" in output and "OBJECT_LOOKUP_REFERENCE 1 1 1 1 1" in native.stdout
