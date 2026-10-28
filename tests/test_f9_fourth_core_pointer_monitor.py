"""Verify bind-proxy pointer fields and native process-monitor no-op dispatch."""

from .support import run_example


def test_f9_fourth_core_pointer_monitor(tmp_path):
    """Check pointer assignment, rejected conversion, native parity and destruction."""
    output = run_example(tmp_path, "tests/models/f9_fourth_core_pointer_monitor.py")
    records = [line for line in output.splitlines() if line.startswith("FOURTH_CORE_POINTER_MONITOR")]
    assert records == ["FOURTH_CORE_POINTER_MONITOR 4 2 0 1"]
