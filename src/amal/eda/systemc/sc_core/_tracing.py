"""Waveform registration and scoped trace-file ownership helpers.

Signals retain native storage; callers keep traced objects alive until closure.
"""

from collections.abc import Iterator
from contextlib import contextmanager
import pathlib
from typing import Any

import cppyy


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
    trace = cppyy.gbl.sc_core.sc_create_vcd_trace_file(str(filename))
    if not trace:
        raise RuntimeError(f"Cannot create VCD trace: {filename}")
    try:
        if time_unit is not None:
            trace.set_time_unit(*time_unit)
        yield trace
    finally:
        cppyy.gbl.sc_core.sc_close_vcd_trace_file(trace)


_native_sc_trace = cppyy.gbl.sc_core.sc_trace


def sc_trace(file: Any, value: Any, name: str, *args: Any) -> Any:
    """Register a named waveform using stable native signal storage.

    Args:
        file: Open SystemC trace file.
        value: Native signal/interface or scalar accepted by SystemC.
        name: Name to record in the waveform.
        *args: Additional native scalar tracing arguments.

    Signal template types are instantiated on demand. Keep the traced object
    alive until the trace file closes; Python scalar temporaries are unsuitable
    for tracking mutable native state.
    """
    if hasattr(value, "read"):
        sample = value.read()
        value_type = bool if isinstance(sample, bool) else type(sample)
        return cppyy.gbl.sc_core.trace_signal[value_type](file, value, name)
    return _native_sc_trace(file, value, name, *args)
