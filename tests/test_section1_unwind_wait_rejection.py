"""Verify expression-list ownership during self-reset and self-kill unwind."""

from .support import run_example


def test_section1_unwind_wait_rejection(tmp_path):
    """Assert report identity, list deletion, lifecycle ordering and no stray wake."""
    output = run_example(tmp_path, "tests/models/section1_unwind_wait_rejection.py")
    assert "SECTION1_UNWIND_WAIT reset_starts=2 kill_starts=1 reports=2 wait_deletes=2 stray_wakes=0" in output


def test_section1_unwind_wait_suppressed_report(tmp_path):
    """Assert suppressed reports reclaim lists and preserve reset/kill unwind."""
    output = run_example(tmp_path, "tests/models/section1_unwind_wait_rejection.py", ("--suppress",))
    assert (
        "SECTION1_UNWIND_WAIT reset_starts=2 kill_starts=1 reports=2 wait_deletes=2 stray_wakes=0 action=suppressed"
        in output
    )
