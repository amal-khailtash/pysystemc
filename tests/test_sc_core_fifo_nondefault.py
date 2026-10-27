"""Compare guarded FIFO reads for a non-default-constructible value with C++."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_fifo_nondefault_value_and_output_reads(tmp_path):
    """Verify independent value returns and staged output assignment against C++."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_output = run_example(python_dir, "tests/models/sc_core_fifo_nondefault.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_fifo_nondefault.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in python_output.splitlines() if line.startswith("FIFO_NONDEFAULT_REFERENCE ")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("FIFO_NONDEFAULT_REFERENCE ")]
    assert actual == expected == ["FIFO_NONDEFAULT_REFERENCE 17 23 1 1"]
