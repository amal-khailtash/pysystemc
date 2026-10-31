"""Check that public trace errors leave the native waveform usable."""

from .support import run_example


def test_trace_error_recovery(tmp_path):
    """Check printf, port-width and selected-overload recovery in a fresh kernel."""
    output = run_example(tmp_path, "tests/models/trace_error_recovery.py")
    assert "TRACE_ERROR_RECOVERY_OK" in output
