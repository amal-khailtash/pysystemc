"""Check original B3 compound/transport adapter destruction lifecycle."""

from .support import run_example


def test_transport_native_teardown_lifecycle(tmp_path):
    """Assert native end-hook usability, owner collection and registry removal."""
    assert "TRANSPORT_TEARDOWN_OK 5 2 ns" in run_example(tmp_path, "tests/models/transport_teardown.py")
