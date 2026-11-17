"""Exercise native enum formatting through its numeric public representation."""

from .support import run_example


def test_native_to_string_fallback(tmp_path):
    """Compare the public fallback with cppyy's selected native formatter."""
    output = run_example(tmp_path, "tests/models/native_overload_selection.py")
    assert "NATIVE_OVERLOAD_SELECTION_OK" in output
