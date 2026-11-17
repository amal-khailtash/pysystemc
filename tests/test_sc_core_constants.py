"""Compare selected sc_core constants with independent native reference output."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_public_constants(tmp_path):
    """Compare 239 declaration records for 238 symbols, retaining redeclarations."""
    output = run_example(tmp_path, "tests/models/sc_core_constants.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_constants.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    records = [line for line in output.splitlines() if line.startswith("CORE_CONSTANT ")]
    assert records == [line for line in native.stdout.splitlines() if line.startswith("CORE_CONSTANT ")]
    assert len(records) == 239
    assert "CORE_ENUM_TYPES_OK 10 48" in output
