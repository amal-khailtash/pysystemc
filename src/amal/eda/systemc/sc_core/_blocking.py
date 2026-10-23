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


class _ScopedLockMeta(type):
    """Recognize the native lock returned by guarded construction."""

    def __getattr__(cls, name):
        """Expose native type metadata and unbound methods on the factory class."""
        return getattr(cppyy.gbl.sc_core.sc_scoped_lock, name)

    def __instancecheck__(cls, instance):
        """Return whether an object is a native scoped lock."""
        return isinstance(instance, cppyy.gbl.sc_core.sc_scoped_lock)


class ScopedLock(metaclass=_ScopedLockMeta):
    """Construct a native owning lock with safe scheduler suspension.

    The mutex is retained until the native lock is destroyed. Call ``release()``
    for explicit idempotent unlocking; otherwise native destruction unlocks it.
    Construction must occur in a SystemC process when acquisition can block.
    Native reports propagate, and reset/kill unwinds the constructing Python
    process before returning control to the scheduler.
    """

    lockable_type = cppyy.gbl.sc_core.sc_mutex_if

    def __new__(cls, mutex):
        """Acquire ``mutex`` and return an owning native ``sc_scoped_lock``.

        Args:
            mutex: Native mutex interface retained for the returned lock lifetime.

        Returns:
            Native lock with the familiar ``release()`` operation.

        Raises:
            sc_report: If native acquisition reports an error.
            TypeError: If the argument does not implement the mutex interface.
        """
        from . import _ProcessUnwind

        bridge = cppyy.gbl.sc_core.python_scoped_lock
        bridge.__release_gil__ = True
        result = bridge(mutex)
        if not result.second:
            raise _ProcessUnwind()
        lock = cppyy.bind_object(result.first, cppyy.gbl.sc_core.sc_scoped_lock)
        lock.__python_owns__ = True
        lock._mutex_owner = mutex
        return lock
