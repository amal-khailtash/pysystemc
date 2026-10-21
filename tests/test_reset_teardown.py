"""Check original B3 teardown after native reset/kill callback unwind."""

from .support import run_example


def test_interface_teardown_after_reset_kill(tmp_path):
    """Assert six paths clean up and terminate before native deletion and owner GC."""
    assert "RESET_TEARDOWN_OK 6 4 ns" in run_example(tmp_path, "tests/models/reset_teardown.py")
