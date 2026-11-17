"""Run typed-token member policy checks in a fresh SystemC process."""

from .support import run_example


def test_member_pointer_policy_values(tmp_path):
    """Compare token-constructed policy access with the native policy reference."""
    output = run_example(tmp_path, "tests/models/member_pointer_policy_values.py")
    records = [line for line in output.splitlines() if line.startswith("MEMBER_POINTER_POLICY ")]
    assert records == ["MEMBER_POINTER_POLICY wide=137 native=1 copy=1 const=1 mutation=1 lifetime=1 recovery=1"]
