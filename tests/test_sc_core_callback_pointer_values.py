"""Validate exact native callback value adapters in an isolated kernel."""

from .support import run_example


def test_native_callback_pointer_values(tmp_path):
    """Check pointer identity, typed consumers, lifetime, and signature rejection."""
    output = run_example(tmp_path, "tests/models/sc_core_callback_pointer_values.py")
    assert "CALLBACK_POINTER_VALUES exact null copy signatures sort map queue ownership report 1" in output
