"""Compare Python FIFO stream insertion against native overload selection."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_fifo_streams(tmp_path):
    """Check generic samples, derived streams, chaining and stream exceptions."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    output = run_example(python_dir, "tests/models/sc_core_fifo_streams.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_fifo_streams.cpp")],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("FIFO_STREAM")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("FIFO_STREAM")]
    assert len(actual) == 6 and actual == expected
    assert (python_dir / "fifo.txt").read_text() == (native_dir / "fifo.txt").read_text() == "17\n23\n"
