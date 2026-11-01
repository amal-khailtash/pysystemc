"""Run typed pointer-to-member assembly checks in a fresh SystemC process."""

from .support import run_example


def test_native_member_pointer_assembly(tmp_path):
    """Check typed tokens against native pointer identity and wide value references."""
    output = run_example(tmp_path, "tests/models/native_member_pointer_assembly.py")
    records = [line for line in output.splitlines() if line.startswith("MEMBER_POINTER_TOKEN ")]
    assert records == [
        "MEMBER_POINTER_TOKEN wide=137 values=2 routes=2 identity=1 copy=1 lifetime=1 binding=1 recovery=1"
    ]
