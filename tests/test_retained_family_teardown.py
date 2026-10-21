"""Verify the complete registered structural adapter family teardown inventory."""

from .support import run_example


def test_retained_family_teardown(tmp_path):
    """Check borrower rejection, owner/result collection and native registry removal."""
    assert "RETAINED_FAMILY_TEARDOWN_OK" in run_example(tmp_path, "tests/models/retained_family_teardown.py")
