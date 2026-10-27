"""Exercise malformed public report-policy calls in an isolated kernel."""

from .support import run_example


def test_sc_core_report_policy_arguments(tmp_path):
    """Check rejected policy calls preserve state and a later report dispatches."""
    output = run_example(tmp_path, "tests/models/sc_core_report_policy_arguments.py")
    assert "REPORT_POLICY_ARGUMENT_REJECTIONS 41" in output
    assert "REPORT_POLICY_ARGUMENT_RECOVERY policy/arguments/recovery 1" in output
