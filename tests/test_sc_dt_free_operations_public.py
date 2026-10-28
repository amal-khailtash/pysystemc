"""Compare free datatype operation overloads against independent native C++."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_dt_free_operations_match_native(tmp_path):
    """Verify scalar/fixed overloads, owned complements and compound mutations."""
    actual = run_example(tmp_path, "tests/models/sc_dt_free_operations_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_free_operations_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    p = [line for line in actual.splitlines() if line.startswith("DT_FREE ")]
    n = [line for line in native.stdout.splitlines() if line.startswith("DT_FREE ")]
    assert len(p) == len(n) == 288
    assert p == n
    assert "SC_DT_FREE_OPERATIONS_PUBLIC_OK" in actual
    assert "SC_DT_FREE_OPERATIONS_PUBLIC_OK" in native.stdout
