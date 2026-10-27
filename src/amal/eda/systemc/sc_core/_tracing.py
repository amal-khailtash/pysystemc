"""Waveform registration and scoped trace-file ownership helpers.

Signals retain native storage; callers keep traced objects alive until closure.
"""

from collections.abc import Iterator
from contextlib import contextmanager
import pathlib
from typing import Any

import cppyy

_open_trace_files = {}


def _trace_create(create):
    """Retain Python-created native trace files until their matching close call."""

    def wrapped(filename):
        """Create a native trace file retained until its matching close call.

        Args:
            filename: String filename stem; native format suffix is appended.

        Returns:
            Borrowed native trace-file proxy retained by the open-file registry.

        Raises:
            TypeError: Filename cannot convert to a native character string.
            sc_report: Native file creation or setup reports.

        The GIL remains held. Keep traced storage alive and close the returned
        file with the matching VCD/WIF helper exactly once after simulation.
        Native files may be opened lazily on the first traced cycle.
        """
        trace = create(filename)
        if trace:
            _open_trace_files[cppyy.addressof(trace)] = trace
        return trace

    return wrapped


def _trace_close(close):
    """Forget a trace only after native closure succeeds."""

    def wrapped(trace):
        """Close a matching native trace file and release its registry entry.

        Args:
            trace: Live trace proxy from the matching create helper, or None.

        Returns:
            None after native destruction flushes pending output.

        Raises:
            TypeError: Argument cannot convert to a native trace-file pointer.
            sc_report: Native closure reports.

        The GIL remains held. This destroys storage, invalidating all borrowed
        aliases. Close non-null files exactly once; mismatched format or repeated
        closure violates the native lifetime contract. Null closure is a no-op.
        """
        address = 0 if trace is None else cppyy.addressof(trace)
        close(cppyy.nullptr if trace is None else trace)
        _open_trace_files.pop(address, None)
        if address:
            trace.__dict__.pop("_trace_value_owners", None)

    return wrapped


sc_create_vcd_trace_file = _trace_create(cppyy.gbl.sc_core.sc_create_vcd_trace_file)
sc_create_wif_trace_file = _trace_create(cppyy.gbl.sc_core.sc_create_wif_trace_file)
sc_close_vcd_trace_file = _trace_close(cppyy.gbl.sc_core.sc_close_vcd_trace_file)
sc_close_wif_trace_file = _trace_close(cppyy.gbl.sc_core.sc_close_wif_trace_file)


@contextmanager
def vcd_trace(filename: str | pathlib.Path, *, time_unit: tuple[float, Any] | None = None) -> Iterator[Any]:
    """Open a VCD trace and close it on normal exit or a Python exception.

    Args:
        filename: Native trace filename stem (SystemC appends .vcd).
        time_unit: Optional (value, sc_time_unit) pair for trace timestamps.

    Yields:
        Native sc_trace_file accepted by sc_trace().

    Raises:
        RuntimeError: If the kernel cannot create the trace file.
        Exception: Exceptions from the body propagate after trace closure.

    Retain traced signals until the context exits. Closing a trace does not
    stop the kernel; independent simulations still require new subprocesses.
    Native process aborts cannot execute Python cleanup.
    """
    trace_path = pathlib.Path(str(filename) + ".vcd")
    if not trace_path.parent.is_dir():
        raise RuntimeError(f"Cannot create VCD trace: directory does not exist: {trace_path.parent}")
    trace = sc_create_vcd_trace_file(str(filename))
    if not trace:
        raise RuntimeError(f"Cannot create VCD trace: {filename}")
    try:
        if time_unit is not None:
            trace.set_time_unit(*time_unit)
        yield trace
    finally:
        sc_close_vcd_trace_file(trace)


_native_sc_trace = cppyy.gbl.sc_core.sc_trace


def sc_trace(file: Any, value: Any, name: str, *args: Any) -> Any:
    """Register a named waveform using stable native signal storage.

    Args:
        file: Open SystemC trace file.
        value: Native signal/interface, sc_trace_value[Type] holder, or native
            scalar reference accepted by SystemC.
        name: Name to record in the waveform.
        *args: Optional native width or holder enumeration label sequence.

    Returns:
        None after native registration.

    Raises:
        TypeError: Arguments lack a compatible native tracing overload.
        sc_report: Native tracing rejects registration.

    Calls hold the GIL. Signal and holder types are instantiated on demand.
    Holders and copied enum labels are retained until matching closure. Keep
    ordinary traced objects alive until closure; Python scalar temporaries are
    unsuitable for tracking mutable native state.
    """
    from ._trace_values import register, value_type

    if value_type(value) is not None:
        return register(file, value, name, *args, native_free=True)
    if hasattr(value, "read"):
        signal_type = None
        for cls in type(value).__mro__:
            native_name = getattr(cls, "__cpp_name__", "")
            for prefix, method in (("sc_core::sc_in<", "input"), ("sc_core::sc_inout<", "inout")):
                if native_name.startswith(prefix):
                    if args:
                        raise TypeError("native port tracing does not accept width arguments")
                    datatype = native_name[len(prefix) : -1]
                    dispatch = cppyy.gbl.pysystemc_tracing.port_dispatch[datatype]
                    return getattr(dispatch, method)(file, value, name)
            if native_name.startswith(
                ("sc_core::sc_signal_in_if<", "sc_core::sc_signal_inout_if<", "sc_core::sc_signal<")
            ):
                arguments = native_name.split("<", 1)[1].removesuffix(">")
                depth = 0
                for index, char in enumerate(arguments):
                    depth += (char == "<") - (char == ">")
                    if char == "," and depth == 0:
                        arguments = arguments[:index]
                        break
                signal_type = arguments.strip()
                break
        if signal_type is None:
            # Do not probe arbitrary read() methods: FIFO reads consume samples
            # and user-defined readers may block or have other side effects.
            return _native_sc_trace(file, value, name, *args)
        if signal_type == "long double":
            raise TypeError("native SystemC tracing does not provide a long double overload")
        if args and signal_type not in ("char", "short", "int", "long"):
            raise TypeError("native signal tracing accepts explicit widths only for char, short, int and long")
        return cppyy.gbl.pysystemc_tracing.signal_dispatch[signal_type].trace(file, value, name, *args)
    return _native_sc_trace(file, value, name, *args)


# Keep exact native pointer/reference overload selection available.
sc_trace.__overload__ = _native_sc_trace.__overload__
