"""Test native reference results from public declared interfaces."""

from .support import run_example


def test_declared_reference_lifetime(tmp_path):
    """Preserve native object identity and callback behavior through simulation."""
    output = run_example(tmp_path, "tests/models/declared_reference_lifetime.py")
    assert "DECLARED_REFERENCE_LIFETIME_OK" in output
