"""Guard native mutex/semaphore suspension through Python process frames."""

import cppyy


def guard_native_channels():
    """Install native blocking wrappers preserving status, errors and reset cleanup.

    Calls borrow the channel for their duration and release the GIL while native
    scheduling runs. Python subclasses may override the familiar native methods.
    """
    native = cppyy.gbl.sc_core
    for classes, method, bridge in (
        ((native.sc_mutex_if, native.sc_mutex), "lock", native.python_mutex_lock),
        ((native.sc_semaphore_if, native.sc_semaphore), "wait", native.python_semaphore_wait),
    ):
        bridge.__release_gil__ = True

        def guarded(self, _bridge=bridge):
            """Return native status or unwind Python frames before kernel rethrow."""
            from . import _ProcessUnwind

            result = _bridge(self)
            if not result.second:
                raise _ProcessUnwind()
            return result.first

        for cls in classes:
            setattr(cls, method, guarded)
