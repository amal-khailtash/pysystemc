"""Check static finder lifetime boundaries in a fresh kernel process."""

from .support import run_example


def test_sc_core_finder_elaboration_lifetime(tmp_path):
    """Retain through elaboration, release, then verify timed sensitivity."""
    output = run_example(tmp_path, "tests/models/sc_core_finder_elaboration_lifetime.py")
    assert "FINDER_ELABORATION_LIFETIME_OK retained=1 released=1 transitions=1000,2000" in output
