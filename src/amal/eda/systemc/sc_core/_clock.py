"""Select native clock time-unit constructors without legacy overload coercion."""

import cppyy


def install():
    """Preserve the native clock class and explicitly select its unit overload."""
    native = cppyy.gbl.sc_core
    original = native.sc_clock.__init__
    short = original.__overload__("const char*, double, sc_core::sc_time_unit, double")

    def initialize(self, *args, **kwargs):
        """Construct a clock with native arguments, preserving explicit time units.

        Args:
            *args: Native constructor arguments, including optional duty cycle,
                start time and first-edge polarity.
            **kwargs: Native constructor keyword arguments.

        Returns:
            None after constructing an owning native clock proxy.

        Raises:
            sc_report: Native clock validation reports an invalid configuration.
            TypeError: Arguments do not match a native constructor.

        GIL stays held during construction. Retain the clock through all bound
        port and process lifetimes; construction schedules its native edge events.
        """
        if len(args) in (3, 4) and isinstance(args[2], native.sc_time_unit):
            return short(self, *args, **kwargs)
        return original(self, *args, **kwargs)

    initialize.__doc__ += "\n\n" + original.__doc__
    native.sc_clock.__init__ = initialize
