"""Compare selected Python adapter edges with an independent native model."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def _records(output):
    """Return the nonempty stable adapter records from a simulation output."""
    return [line for line in output.splitlines() if line.startswith("CORE_")]


def test_core_small_adapter_edges_match_native(tmp_path):
    """Check template call styles, vector re-adaptation, conversions, and start errors."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_output = run_example(python_dir, "tests/models/core_small_adapter_edges.py")
    environment = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/core_small_adapter_edges.cpp"),
        ],
        cwd=native_dir,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = _records(python_output)
    expected = _records(native.stdout)
    assert len(actual) == len(expected) == 5
    assert actual == expected
