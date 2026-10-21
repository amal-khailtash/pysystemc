"""Verify B4 structural and Python-subclass callback error/reset compatibility."""

from .support import run_example


def test_custom_channel_reset_and_errors(tmp_path):
    """Assert native virtual error propagation, GIL progress and cleanup timing."""
    assert "CUSTOM_CHANNEL_RESET_OK 2 4 ns" in run_example(tmp_path, "tests/models/custom_channel_reset.py")
