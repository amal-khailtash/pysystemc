"""Regression for explicitly selecting the public hash-base constructor."""

from .support import run_example


def test_hash_constructor_selection(tmp_path):
    """Select the real native constructor and verify callback ownership."""
    output = run_example(tmp_path, "tests/models/hash_constructor_selection.py")
    assert "HASH_CONSTRUCTOR_SELECTION selected=1 callbacks=1 recovered=1" in output
