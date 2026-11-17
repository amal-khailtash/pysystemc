"""Verify integer and vector owners survive temporary proxy sources."""

from .support import run_example


def test_integer_temporary_sources_are_copied(tmp_path):
    """Check range and concat copies retain full values after source GC."""
    output = run_example(tmp_path, "tests/models/integer_temporary_sources.py")
    assert "INTEGER_TEMPORARY_SOURCES_OK integer=4 vector=4 width=137" in output
