"""Compare public sc_plist linked-list behavior with a native reference."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_lists_public(tmp_path):
    """Verify list mutations, both iterator families, map callbacks, and borrowing."""
    expected = "SC_PLIST_REFERENCE base=5,10,16,40 typed=1,5 map=30,3 borrowed=1"
    output = run_example(tmp_path, "tests/models/sc_core_lists_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_lists_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert expected in output
    assert expected in native.stdout
