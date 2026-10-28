"""Verify native comparator consumers separately from public typedef exposure."""

from .support import run_example


def test_comparator_token_and_pointer_values(tmp_path):
    """Check callable results and exact pointer-value alias identity in a subprocess."""
    output = run_example(tmp_path, "tests/models/f9_fifth_core_dt_comparator_alias.py")
    assert "COMPARATOR_ALIAS_VALUES 2 TOKEN_RESULTS 23 23" in output
