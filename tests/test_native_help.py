"""Acceptance test for the public native API help formatter."""

from .support import run_example


def test_native_help_preserves_native_metadata_and_kernel_state(tmp_path):
    """Exercise native help through package exports and the actual REPL namespace."""
    output = run_example(tmp_path, "tests/models/native_help.py")
    assert "NATIVE_HELP_ACCEPTANCE_OK" in output
