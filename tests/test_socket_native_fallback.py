"""Verify public TLM native fallback rejection and recovery behavior."""

from .support import run_example


def test_socket_native_fallback_rejects_then_recovers(tmp_path):
    """Check fallback rejection, adapter recovery and borrowed owner retention.

    The native member-pointer proxy reaches the original registration overload,
    where cppyy currently reports that this native method cannot be called.
    The model then uses the public exact-pointer holder and an independent
    native initiator to verify callback execution and transaction mutation.
    """
    output = run_example(tmp_path, "tests/models/socket_native_fallback.py")
    assert "SOCKET_NATIVE_FALLBACK_BIND_REJECTED_RECOVERED" in output
    assert "SOCKET_NATIVE_FALLBACK_CALLBACK_REJECTED_RECOVERED" in output
    assert "SOCKET_NATIVE_FALLBACK_RESULT 2 ns 2 ns 1 BEGIN_RESP 3 ns 116" in output
    assert "SOCKET_NATIVE_FALLBACK_OWNER_RETAINED" in output
