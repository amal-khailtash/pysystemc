"""Compare runtime native member-pointer capture with direct C++ assemblies."""

from .support import ROOT, run_example


def test_member_pointer_symbols(tmp_path):
    """Check current symbol values, typed conversion, errors and retained owners."""
    python_output = run_example(tmp_path, "tests/models/member_pointer_symbols.py")
    native_output = run_example(
        tmp_path, "examples/run_sysc_cpp.py", [str(ROOT / "tests/native/member_pointer_symbols.cpp")]
    )
    python_rows = [line for line in python_output.splitlines() if line.startswith("MEMBER_SYMBOL_NATIVE ")]
    native_rows = [line for line in native_output.splitlines() if line.startswith("MEMBER_SYMBOL_NATIVE ")]
    assert len(python_rows) == 1 and python_rows == native_rows
    assert (
        "MEMBER_SYMBOL_PYTHON routes=2 dynamic=1 copy=1 null=1 const=1 inheritance=1 errors=1 recovery=1 lifetime=1"
        in python_output
    )
