"""Check native FIFO blocking scheduler unwind through Python frames."""

import pytest

from .support import run_example


@pytest.mark.parametrize(
    "mode,access",
    [
        (mode, access)
        for mode in ("read", "write")
        for access in ("direct", "port", "convenience", "borrowed", "blocking")
    ]
    + [("write", "assignment"), ("read", "conversion")],
)
def test_native_fifo_guarded_reset(tmp_path, mode, access):
    """Assert direct FIFO reset/kill cleanup, unchanged output and termination."""
    assert "NATIVE_FIFO_GUARDED_RESET_OK" in run_example(
        tmp_path, "tests/models/native_fifo_guarded_reset.py", (mode, access)
    )
