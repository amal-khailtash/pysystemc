"""Check typed spawn storage and callback retention in isolated kernels."""

import pytest

from .support import run_example


@pytest.mark.parametrize("mode", ("normal", "error", "unwind"))
def test_typed_spawn_owner_lifetime(tmp_path, mode):
    """Verify caller GC and result preservation on each native terminal route."""
    output = run_example(tmp_path, "tests/models/f6_spawn_owner_lifetime.py", (mode,))
    assert f"SPAWN_OWNER_LIFETIME_OK mode={mode}" in output
