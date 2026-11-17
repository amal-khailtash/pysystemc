"""Expose native signal conversion operators without changing proxy truthiness."""

import cppyy

_PREFIX = "sc_core::sc_signal_t<"
# Keep only full native type names and free-function dispatch proxies here.
# Signals and their borrowed value references remain owned by their callers.
_value_dispatches = {}
# Cache native callable proxies only; ports and interfaces remain caller-owned.
_port_value_dispatches = {}


def sc_signal_value(signal):
    """Convert a native signal to its current value through its C++ operator.

    Args:
        signal: Live native sc_signal_t-derived signal of any datatype/policy.

    Returns:
        Borrowed read-only native value reference. cppyy copies fundamental
        scalar values into Python; native datatype views reflect later updates.

    Raises:
        TypeError: The argument is not a native signal, or conversion fails.
        ReferenceError: The native signal proxy has been destroyed.

    Calls hold the GIL and do not advance simulation. Keep the signal alive
    while using a borrowed result and use write() to change its value. This
    explicit conversion leaves bool(signal) as cppyy proxy validity and does
    not introduce int(signal) conversions for arbitrary native datatypes.
    """
    for base in type(signal).__mro__:
        name = getattr(base, "__cpp_name__", "")
        if not name.startswith(_PREFIX):
            continue
        dispatch = _value_dispatches.get(name)
        if dispatch is None:
            arguments = name[len(_PREFIX) : -1]
            depth = 0
            for index, character in enumerate(arguments):
                depth += (character in "<([") - (character in ">)]")
                if character == "," and depth == 0:
                    datatype = arguments[:index].strip()
                    policy = f"(sc_core::sc_writer_policy)({arguments[index + 1 :].strip()})"
                    dispatch = cppyy.gbl.pysystemc_signals.value_dispatch[datatype, policy].get
                    _value_dispatches[name] = dispatch
                    break
        if dispatch is not None:
            return dispatch(signal)
    raise TypeError("sc_signal_value requires a live native sc_signal_t-derived signal")


def sc_port_value(port):
    """Convert a bound input, bidirectional or output port to its native value.

    Args:
        port: Live, bound native sc_in, sc_inout or sc_out of any datatype.

    Returns:
        Borrowed read-only value owned by the bound interface. cppyy copies
        fundamental scalars; native datatype views reflect later updates.

    Raises:
        TypeError: The argument is not a supported native port.
        ReferenceError: The native port proxy has been destroyed.

    Bind the port before calling. Native interface access errors propagate.
    Keep the port and its bound interface alive while using borrowed results.
    Calls hold the GIL and do not advance simulation or change proxy truthiness.
    Use the bound channel or a writable port's write() to change its value.
    """
    for base in type(port).__mro__:
        name = getattr(base, "__cpp_name__", "")
        for kind in ("input", "inout"):
            prefix = "sc_core::sc_in<" if kind == "input" else "sc_core::sc_inout<"
            if name.startswith(prefix):
                native = _port_value_dispatches.get(name)
                if native is None:
                    datatype = name[len(prefix) : -1]
                    dispatch = cppyy.gbl.pysystemc_signals.port_value_dispatch[datatype]
                    native = getattr(dispatch, kind)
                    result = native(port)
                    _port_value_dispatches[name] = native
                    return result
                return native(port)
    raise TypeError("sc_port_value requires a live native sc_in/sc_inout/sc_out port")
