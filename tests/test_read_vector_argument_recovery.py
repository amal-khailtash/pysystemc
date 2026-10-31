"""Check public read callback, vector creator, and interface binding recovery."""

from .support import run_example


def test_read_vector_argument_recovery(tmp_path):
    """Recover from rejected public arguments and consume native values."""
    output = run_example(tmp_path, "tests/models/read_vector_argument_recovery.py")
    assert "READ_VECTOR_ARGUMENT_RECOVERY_OK read=37/1 vector=66/3 custom=91" in output
