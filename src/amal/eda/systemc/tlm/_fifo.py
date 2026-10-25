"""Preserve native FIFO overloads and Python cleanup during blocking operations."""

from pathlib import Path

import cppyy

from ..sc_core import _ProcessUnwind

cppyy.include(str(Path(__file__).with_name("fifo_operations.h")))


def _pythonize_fifo(fifo_type, name):
    """Guard native FIFO subclasses without constraining their payload types."""
    if vars(fifo_type).get("_guarded_tlm_fifo", False):
        return
    for base in fifo_type.__mro__:
        native = getattr(base, "__cpp_name__", "")
        prefix = "tlm::tlm_fifo<"
        if native.startswith(prefix):
            payload = native[len(prefix) : -1].strip()
            break
    else:
        return
    operations = cppyy.gbl.pysystemc_interfaces.guarded_tlm_fifo[payload]
    for method in ("get_value", "get_output", "peek_value", "peek_output", "put"):
        getattr(operations, method).__release_gil__ = True
    for method in ("get", "peek"):
        native_call = getattr(fifo_type, method)
        value_call = getattr(operations, method + "_value")
        output_call = getattr(operations, method + "_output")

        def read(owner, *args, _value=value_call, _output=output_call, _native=native_call):
            """Get or peek through the exact native blocking interface.

            Args:
                args: No arguments or a native ``tlm_tag`` pointer for a value
                    result, or one caller-owned mutable output object.
            Returns:
                A native value for the value form; None for the output form.
                Peek preserves the queued transaction; get consumes it.
            Raises:
                TypeError: Native argument conversion or overload selection fails.
                Native exception: A value copy or callback fails.
            Call from a SystemC thread. Retain the FIFO and storage during
            suspension. Blocking calls release the GIL; reset/kill executes
            Python cleanup before native scheduler unwind. Native preconditions
            and mutations remain unchanged; no rollback is promised.
            """
            tag = len(args) == 1 and (
                args[0] is None
                or args[0] is cppyy.nullptr
                or getattr(type(args[0]), "__cpp_name__", "").startswith("tlm::tlm_tag<")
            )
            if not args or tag:
                result = _value(owner, *args)
                if not result.second:
                    raise _ProcessUnwind()
                return result.first
            if len(args) == 1:
                if not _output(owner, args[0]):
                    raise _ProcessUnwind()
                return None
            return _native(owner, *args)

        if hasattr(native_call, "__overload__"):
            read.__overload__ = native_call.__overload__
        setattr(fifo_type, method, read)
    native_put = fifo_type.put

    def put(owner, value):
        """Put a borrowed input through the native blocking FIFO interface.

        Args:
            value: Native-compatible value copied into the FIFO when accepted.
        Returns:
            None after native insertion.
        Raises:
            TypeError: The input cannot convert to the native payload type.
            Native exception: Copy, allocation or a callback fails.
        Call from a SystemC thread and retain the FIFO and input during
        suspension. The GIL is released; reset/kill preserves Python cleanup
        before native scheduler unwind. Native mutations are not rolled back.
        """
        if not operations.put(owner, value):
            raise _ProcessUnwind()

    if hasattr(native_put, "__overload__"):
        put.__overload__ = native_put.__overload__
    fifo_type.put = put
    fifo_type._guarded_tlm_fifo = True


cppyy.py.add_pythonization(_pythonize_fifo)
