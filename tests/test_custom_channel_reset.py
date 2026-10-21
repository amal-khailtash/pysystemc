"""Verify B4 structural and Python-subclass callback error/reset compatibility."""

import pytest

from .support import run_example


@pytest.mark.parametrize("kind", ("mutex", "semaphore"))
def test_custom_channel_reset_and_errors(tmp_path, kind):
    """Assert native virtual error propagation, GIL progress and cleanup timing."""
    assert "CUSTOM_CHANNEL_RESET_OK 2 4 ns" in run_example(
        tmp_path, "tests/models/custom_channel_reset.py", () if kind == "mutex" else (kind,)
    )
