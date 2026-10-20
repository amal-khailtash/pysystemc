"""Compare Python master/slave callbacks with independent native interface behavior."""

import subprocess
import sys

from .support import ROOT, run_example


def case_records(output):
    """Return normalized master/slave records in deterministic order."""
    return sorted(line for line in output.splitlines() if line.startswith("MASTER_SLAVE_CASE "))


def test_master_slave_multiword_overloads_match_native_reference(tmp_path):
    """Check exact asymmetric values, output copies, callback counts, and elapsed time."""
    output = run_example(tmp_path, "tests/models/master_slave_type_matrix.py")
    assert "MASTER_SLAVE_TYPE_MATRIX_OK" in output
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/master_slave_type_matrix.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "MASTER_SLAVE_TYPE_MATRIX_NATIVE_OK" in native.stdout
    assert len(case_records(output)) == 12
    assert case_records(output) == case_records(native.stdout)
