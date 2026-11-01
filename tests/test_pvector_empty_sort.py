"""Verify safe public empty-vector sorting and unchanged nonempty callbacks."""

from .support import run_example


def test_pvector_empty_sort(tmp_path):
    """Check both comparator routes on empty and populated native vectors."""
    output = run_example(tmp_path, "tests/models/pvector_empty_sort.py")
    assert "PVECTOR_EMPTY_SORT_OK" in output
