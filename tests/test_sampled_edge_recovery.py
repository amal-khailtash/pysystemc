"""Verify sampled-edge member type errors leave the native process usable."""

from .support import run_example


def test_sampled_edge_recovery(tmp_path):
    """Recover from invalid member edge inputs and observe later clock samples."""
    output = run_example(tmp_path, "tests/models/sampled_edge_recovery.py")
    assert "SAMPLED_EDGE_RECOVERY pos=True@2000 neg=0@6000 invalid=2 kill=3000" in output.splitlines()
