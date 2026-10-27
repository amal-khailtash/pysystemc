"""Verify Python binder value, reference, void and exception result categories."""

from .support import run_example


def test_sc_core_bind_result_categories(tmp_path):
    """Compare native and Python binding semantics in a bounded process."""
    output = run_example(tmp_path, "tests/models/sc_core_bind_result_categories.py")
    assert ("BIND_RESULT_CATEGORIES 19 19 23 23 True True 31 31 0 0 31 1 0 37 37 0 0 37 1 0 True True") in output
