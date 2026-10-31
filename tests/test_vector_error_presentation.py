"""Verify public vector member errors in an isolated cppyy process."""

from .support import run_example


def test_vector_error_presentation(tmp_path):
    """Preserve native causes, callback identity, nested types, and recovery."""
    output = run_example(tmp_path, "tests/models/vector_error_presentation.py")
    assert "VECTOR_ERROR_PRESENTATION errors=6 malformed=3 recovery=1" in output
    assert "inherited=17 nested=13,29 creator_identity=2" in output
