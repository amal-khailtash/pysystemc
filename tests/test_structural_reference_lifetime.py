"""Test structural adapter retention and stopped-kernel release of references."""

from .support import run_example


def test_structural_reference_lifetime(tmp_path):
    """Keep returned wide values alive until the public callback release call."""
    output = run_example(tmp_path, "tests/models/structural_reference_lifetime.py")
    assert "STRUCTURAL_REFERENCE_LIFETIME_OK" in output
