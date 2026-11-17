"""Check public argument failures leave native SystemC APIs usable."""

from .support import run_example


def test_sc_core_argument_recovery(tmp_path):
    """Recover with real callbacks after rejected public API arguments."""
    output = run_example(tmp_path, "tests/models/sc_core_argument_recovery.py")
    assert "ARGUMENT_RECOVERY_OK python_callbacks=4 event=1000 bind=41" in output
