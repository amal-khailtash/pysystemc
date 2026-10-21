"""Check native FIFO blocking scheduler unwind through Python frames."""

import pytest

from .support import run_example


@pytest.mark.parametrize("access", ("direct", "port", "convenience"))
@pytest.mark.parametrize("mode", ("read", "write"))
def test_native_fifo_guarded_reset(tmp_path, mode, access):
    """Assert direct FIFO reset/kill cleanup, unchanged output and termination."""
    assert "NATIVE_FIFO_GUARDED_RESET_OK" in run_example(
        tmp_path, "tests/models/native_fifo_guarded_reset.py", (mode, access)
    )
