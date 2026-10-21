"""Verify structural floating traces contain native declarations and transitions."""

import subprocess
import sys
from .support import ROOT, run_example


def test_structural_float_trace(tmp_path):
    """Compare real-valued trace records with an independent native simulation."""
    assert "STRUCTURAL_FLOAT_TRACE_OK" in run_example(tmp_path, "tests/models/structural_float_trace.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/structural_float_trace.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    traces = [(tmp_path / name).read_text().splitlines() for name in ("structural_float.vcd", "native_float.vcd")]
    records = [[line for line in lines if line.startswith(("$var", "r", "#"))] for lines in traces]
    assert records[0] == records[1]
    assert sum(line.startswith("$var real") for line in records[0]) == 2
    assert sum(line.startswith("r1.25 ") for line in records[0]) == 2
    assert sum(line.startswith("r-2.5 ") for line in records[0]) == 2
    assert "#1" in records[0]
