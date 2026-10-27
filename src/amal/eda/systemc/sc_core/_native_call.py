"""Invoke native C++ blocking functors while preserving Python process cleanup."""

import cppyy


def native_blocking_call(operation):
    """Execute a zero-argument C++ functor with safe reset and kill handling.

    Args:
        operation: Native callable object whose const call operator executes the
            blocking work entirely in C++. Capture arguments in the functor and
            retain any borrowed channels, events, and buffers through this call.

    Returns:
        A native result value copy, or None for a void operation. Borrowed
        storage inside a returned value retains its normal native lifetime.

    Raises:
        TypeError: The argument is not a native callable object, or its native
            signature cannot supply a void result or a copyable value. Pointer,
            reference and move-only returns are rejected before execution.
        sc_report: The native operation reports an error.

    Call from a SystemC thread process. This adapter releases the GIL, runs Python
    finally blocks on reset/kill, and restores the original scheduler exception.
    Do not suppress BaseException or block during unwind cleanup. Ordinary C++
    exceptions propagate. Python callbacks and raw cppyy function objects are
    rejected: wrapping a Python call cannot intercept cppyy's translation of a
    native scheduler exception. Use a C++ functor to call such functions instead.
    """
    if not isinstance(operation, cppyy._backend.CPPInstance) or not cppyy.addressof(operation):
        raise TypeError("native_blocking_call requires a native C++ callable object")
    from . import _ProcessUnwind

    traits = cppyy.gbl.sc_core.python_native_blocking_traits[type(operation)]
    if not traits.supported():
        raise TypeError(
            "native callable must have a const zero-argument call operator returning void or a copyable value"
        )
    bridge = cppyy.gbl.sc_core.python_native_blocking[type(operation)].invoke
    bridge.__release_gil__ = True
    result = bridge(operation)
    if not result.completed:
        raise _ProcessUnwind()
    return result.value()
