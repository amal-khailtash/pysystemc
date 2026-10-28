"""Exercise native entry cleanup after Python report callback failures."""

import pytest

from .support import run_example


@pytest.mark.parametrize(("scenario", "calls"), (("direct", 1), ("report", 1), ("runtime", 1), ("deprecation", 2)))
def test_sc_core_entry_error_lifetime(tmp_path, scenario, calls):
    """Preserve callback exceptions and release entry/report-handler state."""
    output = run_example(tmp_path, "tests/models/sc_core_entry_error_lifetime.py", (scenario,))
    assert f"ENTRY_ERROR_LIFETIME_OK {scenario} calls={calls} reset=1 released=1" in output
