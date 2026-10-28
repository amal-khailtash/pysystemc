"""Distinguish public report pointer values from unavailable native reflection."""

from .support import run_example


def test_f9_core_next_handler_alias(tmp_path):
    """Compile the native signature and check its exact pointer-value adapter."""
    output = run_example(tmp_path, "tests/models/f9_core_next_handler_alias.py")
    assert "F9_CORE_NEXT_HANDLER_ALIAS 1 1 0" in output
