"""Select native scheduler overloads and guard sampled-edge suspension."""

import cppyy

_UNSET = object()


def sc_start(duration=_UNSET, unit=_UNSET, p=_UNSET):
    """Advance simulation using one selected native overload.

    Args:
        duration: Native sc_time or numeric duration; omit to run without a bound.
        unit: Native time unit for numeric durations, or the policy after sc_time.
        p: Starvation policy, defaulting to SC_RUN_TO_TIME for bounded calls.

    Returns:
        None after native simulation returns.

    Raises:
        TypeError: Arguments do not match a native overload.
        sc_report: The first native simulation report, without retrying entry.

    Calls release the GIL. Kernel state is retained between bounded calls; use
    independent processes for independent simulations. Process callbacks retain
    their existing lifetime and GIL requirements.
    """
    native = cppyy.gbl.sc_core
    bridge = native.python_start
    if duration is _UNSET:
        if unit is not _UNSET or p is not _UNSET:
            raise TypeError("unbounded sc_start takes no time unit or policy")
        operation = bridge.start_default
        arguments = ()
    elif isinstance(duration, native.sc_time):
        if unit is not _UNSET and p is not _UNSET:
            raise TypeError("sc_time duration accepts one starvation policy")
        policy = unit if unit is not _UNSET else p if p is not _UNSET else native.SC_RUN_TO_TIME
        operation = bridge.start_time
        arguments = (duration, policy)
    else:
        if unit is _UNSET:
            raise TypeError("numeric sc_start duration requires a native time unit")
        operation = (
            bridge.start_int if isinstance(duration, int) and -(2**31) <= duration < 2**31 else bridge.start_double
        )
        arguments = (duration, unit, native.SC_RUN_TO_TIME if p is _UNSET else p)
    operation.__release_gil__ = True
    report_address = operation(*arguments)
    if report_address:
        bridge.throw_report(report_address)
    return None


def _edge(signal, simc, positive):
    """Select an interface overload and preserve native scheduler unwind."""
    native = cppyy.gbl.sc_core
    datatype = bool if isinstance(signal, native.sc_signal_in_if[bool]) else cppyy.gbl.sc_dt.sc_logic
    bridge = native.python_edge_wait[datatype]
    operation = bridge.positive if positive else bridge.negative
    operation.__release_gil__ = True
    if simc is None:
        simc = native.sc_get_curr_simcontext()
    if not operation(signal, simc):
        from . import _ProcessUnwind

        raise _ProcessUnwind()


def at_posedge(signal, simc=None):
    """Wait for the native sampled positive edge of a bool or logic interface.

    Args:
        signal: Borrowed native bool/sc_logic input interface, retained during wait.
        simc: Borrowed native simulation context; defaults to the current context.

    Returns:
        None after native sampling completes.

    Raises:
        TypeError: Signal or context cannot match a native overload.
        sc_report: Native invalid process/wait report.

    Run in a SystemC thread with static sensitivity. Calls release GIL, preserve
    native X/Z sampling semantics and unwind Python cleanup on reset/kill.
    """
    return _edge(signal, simc, True)


def at_negedge(signal, simc=None):
    """Wait for the native sampled negative edge of a bool or logic interface.

    Args:
        signal: Borrowed native bool/sc_logic input interface, retained during wait.
        simc: Borrowed native simulation context; defaults to the current context.

    Returns:
        None after native sampling completes.

    Raises:
        TypeError: Signal or context cannot match a native overload.
        sc_report: Native invalid process/wait report.

    Run in a SystemC thread with static sensitivity. Calls release GIL and execute
    Python cleanup before restoring native reset/kill unwind.
    """
    return _edge(signal, simc, False)


def timed_out(simc=None):
    """Return whether the current process last resumed because its wait expired.

    Args:
        simc: Borrowed native simulation context; defaults to the current context.

    Returns:
        Native timeout flag, without advancing simulation.

    Raises:
        TypeError: The argument cannot convert to a native context.
        runtime_error: No current process exists in the selected context.

    Call from a running SystemC process while holding GIL. The selected context
    must remain alive; native deprecation reporting is preserved.
    """
    if simc is None:
        simc = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    return cppyy.gbl.sc_core.python_timed_out(simc)
