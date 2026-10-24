"""Select exact native time constructors for string units and legacy scalars."""


def install(time_type):
    """Preserve native time overloads while avoiding constructor retry diagnostics.

    Args:
        time_type: Reflected native sc_time class.

    String units select the exact native constructor so conversion reports retain
    their native exception type. Legacy integer/bool construction selects uint64
    without losing tick precision; floating scalars select double. Calls hold the
    GIL, own the resulting time value and do not retain callbacks or contexts.
    Native elaboration and conversion errors propagate.
    """
    native = time_type.__init__
    string = native.__overload__("double,const char*")
    string_context = native.__overload__("double,const char*,sc_core::sc_simcontext*")
    floating = native.__overload__("double,bool")
    integer = native.__overload__("sc_core::sc_time::value_type,bool")

    def construct(self, *args, **kwargs):
        """Construct an owned native duration with exact string or legacy dispatch.

        Args:
            *args: Native default/copy, value/unit/context or value/scale arguments.
            **kwargs: Native named unit or legacy scale parameter.

        Raises:
            sc_report: Native time conversion or configuration fails.
            TypeError: Arguments do not match a native constructor.

        The context is borrowed during construction. Calls hold the GIL and
        preserve shared native resolution and default-unit lifecycle rules.
        """
        if len(args) == 1 and len(kwargs) == 1:
            for keyword in ("unit", "scale"):
                if keyword in kwargs:
                    args = (*args, kwargs[keyword])
                    kwargs = {}
                    break
        if len(args) in (2, 3) and isinstance(args[1], str):
            return (string if len(args) == 2 else string_context)(self, *args, **kwargs)
        if len(args) == 2 and type(args[1]) is bool:
            if type(args[0]) is int:
                return integer(self, *args, **kwargs)
            if type(args[0]) is float:
                return floating(self, *args, **kwargs)
        return native(self, *args, **kwargs)

    construct.__overload__ = native.__overload__
    time_type.__init__ = construct
