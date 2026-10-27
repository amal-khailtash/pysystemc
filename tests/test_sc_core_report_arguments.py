"""Exercise malformed argument dispatch for report-handler overloads."""

from .support import run_example


def test_sc_core_report_arguments(tmp_path):
    """Check rejected report arguments preserve counters, cache and thresholds."""
    output = run_example(tmp_path, "tests/models/sc_core_report_arguments.py")
    assert "REPORT_ARGUMENT_REJECTIONS 23" in output
    assert "REPORT_ARGUMENT_RECOVERY matrix/arguments/recovery -1234567 1 1" in output
