"""Run the focused resolved-vector assignment return regression in isolation."""

from .support import run_example


def test_f9_seventh_core_signal_assignment_returns(tmp_path):
    """Require all three Python assignments to return their native target reference."""
    output = run_example(tmp_path, "tests/models/f9_seventh_core_signal_assignment_returns.py")
    assert "F9_SEVENTH_SIGNAL_ASSIGNMENT_RETURNS value=1|interface=1|same_type=1" in output
