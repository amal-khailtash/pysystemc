"""Compare free tracing overloads and delayed port waveforms with SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_free_trace_public(tmp_path):
    """Check exact dispatch, native widths, unbound registration and live values."""
    output = run_example(tmp_path, "tests/models/sc_core_free_trace_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_free_trace_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("FREE_TRACE|")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("FREE_TRACE|")]
    assert len(actual) == 83 and actual == expected
    assert "FREE_TRACE_IGNORED 1" in output and "FREE_TRACE_IGNORED 1" in native.stdout
    assert "FREE_TRACE_TIME 2000" in output and "FREE_TRACE_TIME 2000" in native.stdout
    traces = [(tmp_path / name).read_text().splitlines() for name in ("free_trace.vcd", "native_free_trace.vcd")]
    records = [[line for line in trace if line.startswith(("$var", "#", "b", "0", "1"))] for trace in traces]
    assert records[0] == records[1]
    widths = {row.split()[4]: int(row.split()[2]) for row in records[0] if row.startswith("$var")}
    assert widths == {
        "input_before_bind": 32,
        "inout_before_bind": 32,
        "wide_input_before_bind": 137,
        "wide_inout_before_bind": 137,
        "integer_width": 7,
        "wide_generic": 137,
    }
    assert "#1000" in records[0]
    assert any(line.startswith("b" + format((1 << 136) + 257, "b")) for line in records[0])
