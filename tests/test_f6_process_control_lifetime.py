"""Run F6-01 module process-control lifetime checks in a fresh kernel."""

from .support import run_example


def test_f6_process_control_lifetime(tmp_path):
    """Check reset, kill, callback retention, and safe module-cycle release."""
    output = run_example(tmp_path, "tests/models/f6_process_control_lifetime.py")
    assert "F6_01_PROCESS_CONTROL_LIFETIME_OK method=reset/kill thread=reset/kill" in output
    assert "cthread=reset/kill cleanup=exact released=1" in output
