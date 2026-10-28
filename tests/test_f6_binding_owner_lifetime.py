"""Run structural binding owner checks in a fresh native-kernel process."""

from .support import run_example


def test_f6_binding_owner_lifetime(tmp_path):
    """Check adapter reuse, failed binding integrity, and borrower teardown."""
    output = run_example(tmp_path, "tests/models/f6_binding_owner_lifetime.py")
    assert "F6_BINDING_OWNER_OK invalid=unretained reuse=port,export duplicate=126 borrowers=2 released=2" in output
