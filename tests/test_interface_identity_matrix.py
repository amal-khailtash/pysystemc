"""Exercise shared native identity for inherited channel views."""

from .support import run_example


def test_interface_identity_specialization_and_binding_orders(tmp_path):
    """Check typed signal, FIFO and TLM identity, event timing and shared state."""
    output = run_example(tmp_path, "tests/models/interface_identity_matrix.py")
    assert "IDENTITY_MATRIX_SIGNAL 6 1 ns" in output
    assert "IDENTITY_MATRIX_FIFO 6 1 ns" in output
    assert "IDENTITY_MATRIX_TLM 12 1 ns" in output
    assert "IDENTITY_MATRIX_SPECIALIZATIONS 3 101 103 107" in output
    assert "INTERFACE_IDENTITY_MATRIX_OK" in output
