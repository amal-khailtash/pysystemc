"""Test pointer-vector assignment conversion errors and public recovery."""

from .support import run_example


def test_pvector_pointer_recovery(tmp_path):
    """Reject incompatible pointees without mutating slots, then recover."""
    output = run_example(tmp_path, "tests/models/pvector_pointer_recovery.py")
    assert "PVECTOR_POINTER_RECOVERY_OK" in output
