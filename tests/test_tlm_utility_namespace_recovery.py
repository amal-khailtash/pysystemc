"""Exercise utility namespace failures through ordinary Python lookup."""

from .support import run_example


def test_tlm_utility_namespace_recovery(tmp_path):
    """Reject absent and foreign bridge names, then use a native utility."""
    output = run_example(tmp_path, "tests/models/tlm_utility_namespace_recovery.py")
    assert "TLM_UTILITY_NAMESPACE_RECOVERY_OK" in output
