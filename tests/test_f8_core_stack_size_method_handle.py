"""Check native thread-only preconditions on the stack-size bridge."""

from .support import run_example


def test_native_stack_size_bridge_rejects_method_process(tmp_path):
    """A method handle must not be cast to a native thread process."""
    output = run_example(tmp_path, "tests/models/f8_core_stack_size_method_handle.py")
    assert "F8_CORE_METHOD_STACK_REJECTED" in output
