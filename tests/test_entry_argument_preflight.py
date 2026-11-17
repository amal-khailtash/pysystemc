"""Cover main's Python argument preflight and later entry recovery."""

import pytest

from .support import run_example


@pytest.mark.parametrize("scenario", ("nonstring", "nul"))
def test_entry_argument_preflight(tmp_path, scenario):
    """Require malformed argv to fail before native entry and recover after."""
    output = run_example(tmp_path, "tests/models/entry_argument_preflight.py", (scenario,))
    assert f"ENTRY_ARGUMENT_PREFLIGHT_OK scenario={scenario}" in output
