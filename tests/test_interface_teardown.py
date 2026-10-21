"""Verify native structural adapter destruction and conservative borrower safety."""

from .support import run_example


def test_native_interface_teardown(tmp_path):
    """Assert atomic rejection, owner collection and native registry removal."""
    assert "INTERFACE_TEARDOWN_OK 1 1 ns" in run_example(tmp_path, "tests/models/interface_teardown.py")
