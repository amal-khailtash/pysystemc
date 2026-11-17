"""Verify defensive adapter guards in an isolated Python process."""

from .support import run_example


def test_coverage_residual_defenses_run_in_fresh_process(tmp_path):
    """Run native imports and Python guard checks outside pytest's kernel."""
    output = run_example(tmp_path, "tests/models/coverage_residual_defenses.py")
    assert "COVERAGE_RESIDUAL_DEFENSES_OK checks=5" in output
