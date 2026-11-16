"""Validate member next_trigger registration lifetime through method reset/kill."""

from .support import run_example


def test_member_trigger_reset_kill_matches_native(tmp_path):
    """Require native/Python callbacks, reset/kill order and handle parity."""
    output = run_example(tmp_path, "tests/models/sc_core_member_trigger_control.py")
    assert "MEMBER_TRIGGER_CONTROL_OK times=0,2000 reset=2000 kill=3000 final=5000" in output
