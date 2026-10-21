"""Compare all native logic-resolution cells and public configuration reads."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_resolution_table(tmp_path):
    """Verify complete table values and initial mutable-global read exposure."""
    output = run_example(tmp_path, "tests/models/sc_core_resolution_table.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_resolution_table.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = ("RESOLUTION_CELL ", "CORE_GLOBAL_READS ")
    records = [line for line in output.splitlines() if line.startswith(prefixes)]
    assert len(records) == 17
    assert records == [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
