"""Preserve instance binding for exact native overload selectors on adapters."""

from functools import partial


class NativeMethod:
    """Bind a Python adapter and its original native overloads consistently.

    The descriptor retains the original native method and adapter function.
    Bound callables retain their owner exactly as ordinary bound methods do;
    they register no callbacks or additional native ownership.
    """

    def __init__(self, wrapper, original):
        """Retain adapter and native method descriptors without native instances."""
        self.wrapper = wrapper
        self.original = original
        self.__doc__ = wrapper.__doc__
        self.__name__ = wrapper.__name__

    def __call__(self, *args, **kwargs):
        """Invoke the unbound adapter with its explicit owner argument."""
        return self.wrapper(*args, **kwargs)

    def __get__(self, owner, owner_type=None):
        """Return an unbound descriptor or a callable retaining the live owner."""
        if owner is None:
            return self
        bound = partial(self.wrapper, owner)
        bound.__doc__ = self.__doc__
        if hasattr(self.original, "__overload__"):

            def overload(signature):
                """Select an exact native overload already bound to this owner."""
                return partial(self.original.__overload__(signature), owner)

            bound.__overload__ = overload
        return bound

    def __overload__(self, signature):
        """Select an unbound native overload for an explicit owner argument."""
        return self.original.__overload__(signature)
