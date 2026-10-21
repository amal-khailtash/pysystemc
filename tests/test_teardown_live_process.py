"""Verify B3 teardown rejects suspended native processes without mutation."""

from .support import run_example


def test_teardown_rejects_live_process(tmp_path):
    """Assert owner/callback remains live after stopped-kernel process gate rejection."""
    assert "LIVE_PROCESS_TEARDOWN_REJECTED 1 ns" in run_example(tmp_path, "tests/models/teardown_live_process.py")
