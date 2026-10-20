"""Check callback owner retention and explicit stopped-kernel release."""

from .support import run_example


def test_compound_callback_owner_lifetime(tmp_path):
    """Verify four compound adapters retain and release their callback owners."""
    output = run_example(tmp_path, "tests/models/compound_owner_lifetime.py")
    assert "COMPOUND_OWNER_LIFETIME 4 " in output
    assert "COMPOUND_OWNER_LIFETIME_OK" in output
