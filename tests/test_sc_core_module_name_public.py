"""Compare public sc_module_name declarations and lifetime with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_module_name_public(tmp_path):
    """Check constructor, copy, conversion and hierarchy scope lifetime parity."""
    output = run_example(tmp_path, "tests/models/sc_core_module_name_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_module_name_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("MODULE_NAME_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("MODULE_NAME_REFERENCE")]
    assert (
        actual
        == expected
        == [
            "MODULE_NAME_REFERENCE root root root root.child root.child.grandchild "
            "root.child.grandchild.marker root.child.after_grandchild root.after_child outside"
        ]
    )
