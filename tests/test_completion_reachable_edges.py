"""Verify bounded reachable adapter paths against native behavior."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def test_completion_reachable_edges_use_native_references(tmp_path):
    """Check invalid wait validation, warm adapters and native type selection."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    output = run_example(python_dir, "tests/models/completion_reachable_edges.py")
    assert ("COMPLETION_REACHABLE_EDGES_OK wait=TypeError concat=head-tail vector=warm endian=native-type") in output

    environment = dict(os.environ)
    environment.setdefault("SYSTEMC_HOME", str(ROOT / "submodules/systemc"))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/datatype_adapter_edges.cpp"),
        ],
        cwd=native_dir,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    def concat_records(text):
        """Select the integer head/tail concat rows from deterministic output."""
        return [
            line
            for line in text.splitlines()
            if line.startswith("DAE\tCONCAT\t") and line.split("\t")[2] in {"vector_head", "vector_tail"}
        ]

    assert concat_records(output) == concat_records(native.stdout)
