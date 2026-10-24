"""Compare sc_interface public virtual dispatch against native C++."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_interface_public(tmp_path):
    """Verify port registration, borrowed event dispatch and native class traits."""
    output = run_example(tmp_path, "tests/models/sc_core_interface_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_interface_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    expected = [line for line in native.stdout.splitlines() if line.startswith("INTERFACE_REFERENCE ")]
    actual = [line for line in output.splitlines() if line.startswith("INTERFACE_REFERENCE ")]
    assert expected == ["INTERFACE_REFERENCE 1 1 1 1 native_ready"]
    assert actual == ["INTERFACE_REFERENCE 1 1 1 1 python_ready"]
    assert expected[0].rsplit(" ", 1)[0] == actual[0].rsplit(" ", 1)[0]
