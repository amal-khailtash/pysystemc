"""Validate native unsigned report-action masks before policy mutation."""

import ctypes
import operator


_MASK_MAX = (1 << (ctypes.sizeof(ctypes.c_uint) * 8)) - 1


def _action_mask(value):
    """Convert an integer-like mask without silently narrowing native action bits."""
    try:
        mask = operator.index(value)
    except TypeError:
        raise TypeError("report actions require an integer mask") from None
    if not 0 <= mask <= _MASK_MAX:
        raise OverflowError("report actions must fit native unsigned int")
    return mask


def install_report_policy(handler, severity_type, clear_log_file_name, null_pointer):
    """Install checked setters on the native report-handler class.

    Args:
        handler: Native sc_report_handler class after bridge initialization.
        severity_type: Native sc_severity enum used to select policy scope.
        clear_log_file_name: Exact native null filename setter bridge.
        null_pointer: cppyy's native null pointer singleton.

    Returns:
        None. Native overloads remain responsible for all non-mask arguments.

    Native policy owns copied message names and action settings. Call with the
    GIL held on the simulation thread; installation retains no Python callback.
    """
    native_actions = handler.set_actions
    severity_actions = native_actions.__overload__("sc_core::sc_severity, sc_core::sc_actions")
    message_actions = native_actions.__overload__("const char*, sc_core::sc_actions")
    specific_actions = native_actions.__overload__("const char*, sc_core::sc_severity, sc_core::sc_actions")
    native_suppress = handler.suppress
    native_force = handler.force
    native_catch = handler.set_catch_actions
    native_log_name = handler.set_log_file_name

    def set_log_file_name(filename):
        """Set the copied log filename or clear it with None or cppyy.nullptr.

        Args:
            filename: Native-compatible filename, or a null pointer selection.

        Returns:
            True when a new filename is installed. False when a non-null name
            is already configured or the name is cleared, as in native SystemC.

        Raises:
            TypeError: The argument cannot convert to a native filename.

        Call with the GIL held on the simulation thread. Close an active default
        log stream before changing its name. Native policy owns the copied name;
        no Python callback is retained or invoked.
        """
        if filename is None or filename is null_pointer:
            return clear_log_file_name()
        return native_log_name(filename)

    def set_actions(*args):
        """Set severity, message or message/severity native report actions.

        Args:
            *args: Severity/actions, message/actions or message/severity/actions.
                Actions may be omitted to install SC_UNSPECIFIED. Explicit
                actions must be integer-like and fit native unsigned int.

        Returns:
            Previous native action mask for the selected policy scope.

        Raises:
            TypeError: Arity, action type or native non-mask arguments are invalid.
            OverflowError: Actions are negative or exceed native unsigned int.

        Hold the GIL on the simulation thread. Policy owns copied message names;
        rejected masks do not alter policy, report counters, caches or callbacks.
        """
        if len(args) not in (1, 2, 3):
            raise TypeError("set_actions expects severity or message, optional severity, and optional actions")
        if len(args) == 3:
            return specific_actions(args[0], args[1], _action_mask(args[2]))
        if len(args) == 2 and not isinstance(args[0], severity_type) and isinstance(args[1], severity_type):
            return specific_actions(*args)
        operation = severity_actions if isinstance(args[0], severity_type) else message_actions
        if len(args) == 1:
            return operation(args[0])
        return operation(args[0], _action_mask(args[1]))

    def suppress(*args):
        """Set the suppression mask, or reset it to zero when omitted.

        Args:
            *args: Optional integer-like mask fitting native unsigned int.

        Returns:
            Previous native suppression mask.

        Raises:
            TypeError: More than one argument is given or the mask is not integer-like.
            OverflowError: The mask is negative or exceeds native unsigned int.

        Hold the GIL on the simulation thread; no ownership transfers or callbacks
        occur. Rejected masks leave report policy and cached reports unchanged.
        """
        if len(args) > 1:
            raise TypeError("suppress expects an optional action mask")
        return native_suppress(*(_action_mask(args[0]),) if args else ())

    def force(*args):
        """Set forced report actions, or reset them to zero when omitted.

        Args:
            *args: Optional integer-like mask fitting native unsigned int.

        Returns:
            Previous native forced-action mask.

        Raises:
            TypeError: More than one argument is given or the mask is not integer-like.
            OverflowError: The mask is negative or exceeds native unsigned int.

        Hold the GIL on the simulation thread; no ownership transfers or callbacks
        occur. Rejected masks leave report policy and cached reports unchanged.
        """
        if len(args) > 1:
            raise TypeError("force expects an optional action mask")
        return native_force(*(_action_mask(args[0]),) if args else ())

    def set_catch_actions(mask):
        """Set native entry exception actions using a checked unsigned mask.

        Args:
            mask: Integer-like action mask fitting native unsigned int.

        Returns:
            Previous native catch-action mask.

        Raises:
            TypeError: The mask is not integer-like.
            OverflowError: The mask is negative or exceeds native unsigned int.

        Hold the GIL on the simulation thread. Native policy retains the value;
        rejected masks do not change policy or dispatch a report callback.
        """
        return native_catch(_action_mask(mask))

    handler.set_actions = staticmethod(set_actions)
    handler.suppress = staticmethod(suppress)
    handler.force = staticmethod(force)
    handler.set_catch_actions = staticmethod(set_catch_actions)
    handler.set_log_file_name = staticmethod(set_log_file_name)
