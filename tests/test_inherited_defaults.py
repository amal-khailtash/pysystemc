"""Compare inherited native defaults and public template defaults with a C++ oracle."""

import subprocess
import sys

from .support import ROOT, run_example


def test_inherited_defaults(tmp_path):
    """Assert static default selection, virtual values, type identity and elaboration."""
    output = run_example(tmp_path, "tests/models/inherited_defaults.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/inherited_defaults.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    marker = "INHERITED_DEFAULTS_REFERENCE 229 111 211 211 3 1 1 0 0"
    assert marker in output and marker in native.stdout
