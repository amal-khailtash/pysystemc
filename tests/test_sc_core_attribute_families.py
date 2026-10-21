"""Compare attribute family constructors, collection operations and ownership."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_attribute_families(tmp_path):
    """Verify generic types, copied/borrowed identity, iteration and destruction."""
    output = run_example(tmp_path, "tests/models/sc_core_attribute_families.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_attribute_families.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("ATTRIBUTE_FAMILY_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("ATTRIBUTE_FAMILY_REFERENCE")]
    assert len(actual) == 1 and actual == expected
