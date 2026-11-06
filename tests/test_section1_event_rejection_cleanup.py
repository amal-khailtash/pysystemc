"""Verify expression-list cleanup when native scheduler context checks reject calls."""

from .support import run_example


def test_section1_event_rejection_cleanup(tmp_path):
    """Check native list destruction, report fidelity, suppression, and recovery."""
    output = run_example(tmp_path, "tests/models/section1_event_rejection_cleanup.py")
    assert "SECTION1_REJECTED_EXPR wait_deletes=17 trigger_deletes=14 reports=36" in output
    assert "SECTION1_REJECTED_EXPR_RECOVERY method=1 thread=1 reset=1 kill=1 empty=throw/suppress channel=numeric actions=throw/suppress" in output
