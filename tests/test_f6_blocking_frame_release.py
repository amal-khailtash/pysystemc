"""Verify local TLM FIFO argument frames release after reset and kill."""

import pytest

from .support import run_example


@pytest.mark.parametrize("case", ("get-output", "peek-value", "put-full"))
def test_f6_blocking_frame_release(tmp_path, case):
    """Check each direct blocking overload in its own kernel subprocess."""
    working = tmp_path / case
    working.mkdir()
    output = run_example(working, "tests/models/f6_blocking_frame_release.py", (case,))
    assert f"F6_08_BLOCKING_FRAME_RELEASE_OK case={case} reset=2ns kill=3ns refs=6 cleanup=2" in output
