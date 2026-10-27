"""Verify native comparator consumers separately from public typedef exposure."""

from .support import run_example


def test_comparator_token_and_missing_alias(tmp_path):
    """Check callable results and explicit partial alias status in a subprocess."""
    output = run_example(tmp_path, "tests/models/f9_fifth_core_dt_comparator_alias.py")
    assert "COMPARATOR_ALIAS_PARTIAL 2 TOKEN_RESULTS 23 23" in output
