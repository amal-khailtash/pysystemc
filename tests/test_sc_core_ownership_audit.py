"""Check weakly observed ownership for plain dynamic-process callbacks."""

import pytest

from .support import run_example


@pytest.mark.parametrize(
    ("scenario", "expected"),
    (
        ("success", "PLAIN_SPAWN_CALLBACK_LIFETIME_OK calls=1 retained_after_stop=1"),
        ("error", "PLAIN_SPAWN_CALLBACK_ERROR_OK calls=1 retained_after_error=1"),
    ),
)
def test_plain_spawn_callback_ownership(tmp_path, scenario, expected):
    """Assert plain spawn callbacks survive collection, dispatch and terminal paths."""
    output = run_example(tmp_path, "tests/models/sc_core_ownership_audit.py", (scenario,))
    assert expected in output
