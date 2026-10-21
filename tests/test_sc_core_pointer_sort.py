"""Compare native pointer-vector sort with a retained Python CFT callback."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_pointer_sort(tmp_path):
    """Verify callback execution and exact pointee order without ownership transfer."""
    output = run_example(tmp_path, "tests/models/sc_core_pointer_sort.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_pointer_sort.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "POINTER_SORT_REFERENCE 1 2 3" in output and "POINTER_SORT_REFERENCE 1 2 3" in native.stdout
