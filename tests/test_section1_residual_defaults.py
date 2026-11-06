"""Verify the six native context defaults and four fixed-point template defaults."""

from .support import run_example


def test_section1_residual_defaults(tmp_path):
    """Check omitted context calls, borrowed vector identity and fixed-point types."""
    output = run_example(tmp_path, "tests/models/section1_residual_defaults.py")
    assert "SECTION1_RESIDUAL_DEFAULTS context_calls=6 borrowed_vectors=2 templates=4 value=1.5" in output
