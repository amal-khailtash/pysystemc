"""Validate SystemC tutorials in isolated subprocesses."""

import subprocess
import sys

from .support import ROOT, run_example


def test_hello_callback_and_lifecycle(tmp_path):
    """Verify method dispatch and all elaboration/simulation lifecycle hooks."""
    output = run_example(tmp_path, "examples/helloworld/py/helloworld.py")
    assert output.count("- hello2()") == 1
    for event in ("Before End of Elaboration", "Elaboration finished", "Simulation started", "Simulation finished"):
        assert event in output


def test_counter_sensitivity_and_trace(tmp_path):
    """Check edge sensitivity, modulo counter values and actual VCD transitions."""
    output = run_example(tmp_path, "examples/counter/py/counter_tb.py")
    assert output.count("Incremented Counter") == 20
    assert "@74 ns :: Incremented Counter 0" in output
    trace = (tmp_path / "counter.vcd").read_text()
    for name in ("clock", "reset", "enable", "count"):
        assert any(name in line for line in trace.splitlines() if line.startswith("$var"))
    assert "b1111 aaaad" in trace
    assert "b0 aaaad" in trace


def test_fifo_threads_with_pthreads(tmp_path):
    """Validate blocking Python FIFO processes with the default native build."""
    output = run_example(tmp_path, "examples/simple_fifo/py/simple_fifo.py")
    assert "Visit www.accellera.org and see what SystemC can do for you today!" in output


def test_toy_event_cancellation(tmp_path):
    """A cancelled timed notification never invokes the method observer."""
    assert "EVENT_CANCEL_OK" in run_example(tmp_path, "examples/toy/event_cancel.py")


def test_toy_mixed_native_gate(tmp_path):
    """Native ports connected to Python-owned channels preserve gate behavior."""
    assert "MIXED_GATE_OK" in run_example(tmp_path, "examples/toy/mixed_gate.py")


def test_toy_trace_closes_after_exception(tmp_path):
    """A context-managed trace flushes transitions even when its body fails."""
    output = run_example(tmp_path, "examples/toy/trace_cleanup.py")
    assert "TRACE_CLEANUP_OK" in output
    assert (tmp_path / "cleanup.vcd").is_file()


def test_toy_adder_width_and_overflow(tmp_path):
    """Check combinational port propagation and native width truncation."""
    assert "ADDER_OK" in run_example(tmp_path, "examples/toy/adder.py")


def test_toy_up_counter_reset_enable_wrap(tmp_path):
    """Check positive-edge synchronous reset, enable hold and eight-bit wrap."""
    assert "UP_COUNTER_OK" in run_example(tmp_path, "examples/toy/up_counter.py")


def test_counter_matches_native_values_and_trace(tmp_path):
    """Compare every increment and VCD transition with the native tutorial."""
    import re

    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    output = run_example(python_dir, "examples/counter/py/counter_tb.py")
    result = subprocess.run(
        [sys.executable, "-m", "amal.eda.systemc._runner", str(ROOT / "examples/counter/cpp/counter_tb.cpp")],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"@(\d+ ns) :: Incremented Counter (\d+)"
    assert re.findall(pattern, output) == re.findall(pattern, result.stdout)
    assert len(re.findall(pattern, output)) == 20
    python_trace = (python_dir / "counter.vcd").read_text()
    native_trace = (native_dir / "counter.vcd").read_text()
    assert python_trace[python_trace.index("$timescale") :] == native_trace[native_trace.index("$timescale") :]
