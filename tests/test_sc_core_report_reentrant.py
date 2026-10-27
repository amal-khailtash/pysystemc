"""Check report-handler replacement and restoration during callbacks."""

import pytest

from .support import run_example


@pytest.mark.parametrize("scenario", ("replace", "restore"))
def test_sc_core_report_reentrant_handler_lifetime(tmp_path, scenario):
    """Verify reentrant handler lifetime and original callback exceptions."""
    output = run_example(tmp_path, "tests/models/sc_core_report_reentrant.py", (scenario,))
    assert f"REPORT_REENTRANT_OK {scenario}" in output
