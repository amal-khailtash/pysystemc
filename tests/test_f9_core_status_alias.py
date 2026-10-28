"""Validate public native status enum exposure without changing query behavior."""

from .support import run_example


def test_f9_core_status_alias(tmp_path):
    """Check exact cppyy enum identity, typed use, and compatibility state query."""
    output = run_example(tmp_path, "tests/models/f9_core_status_alias.py")
    assert "F9_CORE_STATUS_ALIAS 1 1 1" in output
