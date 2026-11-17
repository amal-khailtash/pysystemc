"""Exercise public native interface argument rejection and recovery."""

from .support import run_example


def test_interface_argument_errors_recover_through_native_dispatch(tmp_path):
    """Check public declaration errors, then call a valid Python override natively."""
    output = run_example(tmp_path, "tests/models/interface_argument_recovery.py")
    assert "INTERFACE_ARGUMENT_RECOVERY_OK declared=73/1 inferred=True/1" in output
