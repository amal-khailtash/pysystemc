"""Check public TLM callback errors leave native dispatch usable."""

from .support import run_example


def test_tlm_callback_errors_recover_in_fresh_kernel(tmp_path):
    """Reject malformed public calls and verify later native callback delivery."""
    output = run_example(tmp_path, "tests/models/tlm_callback_error_recovery.py")
    assert "TLM_CALLBACK_RECOVERY_SOCKET 73 0 s 0 s" in output
    assert "TLM_CALLBACK_RECOVERY_PEQ 601 1 2 ns" in output
    assert "TLM_CALLBACK_RECOVERY_PROTOCOL 73 0 s 0 s" in output
    assert "TLM_CALLBACK_ERROR_RECOVERY_OK" in output
