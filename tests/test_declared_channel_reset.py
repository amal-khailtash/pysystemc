"""Check arbitrary declared-interface blocking methods and callback recovery."""

import pytest

from .support import run_example


@pytest.mark.parametrize("access", ("subclass", "structural"))
def test_declared_channel_reset(tmp_path, access):
    """Verify exact reset/kill cleanup, termination and error recovery."""
    assert "DECLARED_CHANNEL_RESET_OK" in run_example(
        tmp_path, "tests/models/declared_channel_reset.py", () if access == "subclass" else (access,)
    )
