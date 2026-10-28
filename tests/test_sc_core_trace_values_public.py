"""Compare live holder tracing, exact native widths and garbage collection."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_trace_values_public(tmp_path):
    """Verify native VCD declarations/transitions and owner release at closure."""
    output = run_example(tmp_path, "tests/models/sc_core_trace_values_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_trace_values_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "TRACE_VALUES_LIFETIME 7 8 0" in output
    assert "TRACE_VALUES_TIME 2000" in output and "TRACE_VALUES_TIME 2000" in native.stdout
    traces = [(tmp_path / name).read_text().splitlines() for name in ("trace_values.vcd", "native_trace_values.vcd")]
    records = [[line for line in lines if line.startswith(("$var", "#", "r", "b", "0", "1"))] for lines in traces]
    assert records[0] == records[1]
    declarations = [line.split() for line in traces[0] if line.startswith("$var")]
    assert {row[4]: int(row[2]) for row in declarations} == {
        "float_value": 1,
        "double_value": 1,
        "short_value": 9,
        "wide_integer": 64,
        "boolean": 1,
        "state": 2,
        "wide_vector": 137,
    }
    assert "#1000" in records[0]
    assert sum(line.startswith("r") for line in records[0]) == 4
    assert any(line.startswith("b" + format((1 << 136) + 257, "b")) for line in records[0])
