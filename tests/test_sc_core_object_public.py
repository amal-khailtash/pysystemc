"""Compare complete sc_object surface behavior with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_object_public(tmp_path):
    """Verify names, hierarchy, attributes, output, protected helpers and copies."""
    output = run_example(tmp_path, "tests/models/sc_core_object_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_object_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    marker = next(line for line in output.splitlines() if line.startswith("SC_OBJECT_PUBLIC_REFERENCE "))
    assert marker in native.stdout
