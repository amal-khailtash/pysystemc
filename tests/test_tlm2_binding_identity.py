"""Verify standard TLM2 protocol identity and shared callback state."""

from .support import run_example


def test_tlm2_partial_first_binding_identity(tmp_path):
    """Check all standard forward/backward first-bind choices and borrowed outputs."""
    assert "TLM2_BINDING_IDENTITY 5 3 2 ns" in run_example(tmp_path, "tests/models/tlm2_binding_identity.py")


def test_tlm2_custom_partial_first_binding_identity(tmp_path):
    """Check explicit custom protocol sharing and arbitrary-width payload preservation."""
    assert "TLM2_CUSTOM_BINDING_IDENTITY 5 3 2 ns" in run_example(
        tmp_path, "tests/models/tlm2_binding_identity.py", ("custom",)
    )
