"""Guard native module and primitive-channel scheduling member calls."""

import cppyy


def _arguments(arguments, *, cycles):
    """Select native template types once and consume borrowed event expressions."""
    values = []
    types = []
    for index, value in enumerate(arguments):
        name = getattr(type(value), "__cpp_name__", "")
        if name.startswith("sc_core::sc_event_expr<"):
            value = value.release()
            name = getattr(type(value), "__cpp_name__", "")
        if index == 0 and isinstance(value, (int, float)):
            name = "int" if cycles and len(arguments) == 1 and isinstance(value, int) else "double"
        elif index == 1 and isinstance(arguments[0], (int, float)):
            name = "sc_core::sc_time_unit"
        if not name:
            raise TypeError("member scheduling requires native time/event arguments or numeric duration")
        # cppyy typedef spellings are valid template arguments, including lists.
        types.append(name)
        values.append(value)
    return types, values


def _scheduling_member(owner_type, method, unwind):
    """Bind one exact native member adapter to a public bridge class."""
    dispatch = getattr(cppyy.gbl.sc_core, "python_member_" + method)

    def invoke(self, *args):
        """Schedule through this owner's native member without changing context.

        Args:
            *args: Native time, event, OR/AND list or numeric duration arguments;
                wait also accepts an integer cycle count. Expressions are consumed.

        Returns:
            None after completion or registration of method sensitivity.

        Raises:
            TypeError: Arguments do not match a native member overload.
            sc_report: The current process cannot perform the native operation.

        Retain this owner and referenced events throughout suspension. wait
        releases the GIL and executes Python finally blocks on reset/kill before
        restoring native scheduler control. Do not suppress BaseException or
        wait during unwind cleanup. next_trigger holds the GIL without waiting.
        """
        types, values = _arguments(args, cycles=method == "wait")
        operation = dispatch[tuple([owner_type, *types])].invoke
        operation.__release_gil__ = method == "wait"
        result = operation(self, *values)
        if method == "wait" and not result:
            raise unwind()

    return invoke


def install(module_type, channel_type, unwind):
    """Install guarded native scheduling members on exported bridge classes."""
    for owner in (module_type, channel_type):
        for method in ("wait", "next_trigger"):
            setattr(owner, method, _scheduling_member(owner, method, unwind))
        owner.timed_out = _timeout_member(owner)
    dispatch = cppyy.gbl.sc_core.python_member_scheduler[module_type]

    def halt(self):
        """Halt this module's current clocked thread with native Python cleanup.

        Returns no normal result. Native invalid-process reports propagate.
        Retain the module during suspension. The call releases the GIL, executes
        Python finally blocks and restores halt/reset/kill control; do not wait
        during unwind cleanup or suppress BaseException.
        """
        dispatch.halt.__release_gil__ = True
        if not dispatch.halt(self):
            raise unwind()

    module_type.halt = halt

    def edge(method):
        """Bind exact bool/logic sampled-edge member forwarding."""

        def invoke(self, signal):
            """Wait for a sampled bool/logic edge through this native module.

            Args:
                signal: Borrowed native bool or sc_logic input interface.

            Returns:
                None when the sampled edge wait completes.

            Raises:
                TypeError: Signal has no native bool/logic input interface.
                sc_report: Native process or interface access is invalid.

            Retain module/interface throughout suspension. The GIL is released;
            reset/kill runs Python cleanup then restores native control flow.
            Call from a native clocked thread.
            Do not suppress BaseException or wait during unwind cleanup.
            """
            for base in type(signal).__mro__:
                name = getattr(base, "__cpp_name__", "")
                prefix = "sc_core::sc_signal_in_if<"
                if name.startswith(prefix):
                    datatype = name[len(prefix) : -1]
                    if datatype not in ("bool", "sc_dt::sc_logic"):
                        break
                    adapter = cppyy.gbl.sc_core.python_member_edge[module_type, datatype]
                    operation = getattr(adapter, method)
                    operation.__release_gil__ = True
                    if not operation(self, signal):
                        raise unwind()
                    return
            raise TypeError("sampled edge waits require a native bool/sc_logic input interface")

        return invoke

    module_type.at_posedge = edge("posedge")
    module_type.at_negedge = edge("negedge")


def _timeout_member(owner_type):
    """Select the owner's guarded native timeout member."""
    dispatch = cppyy.gbl.sc_core.python_member_timeout[owner_type]

    def timed_out(self):
        """Return the current native process's timeout flag through this owner.

        Returns:
            bool: Whether the most recent scheduled resume was a timeout.

        Raises:
            runtime_error: No current SystemC process exists in this context;
                the native std::runtime_error is preserved.

        Keep the native owner alive. This call holds the GIL without advancing
        simulation and guards invalid context before native pointer access.
        """
        return bool(dispatch.invoke(self))

    return timed_out
