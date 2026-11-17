"""Exercise scheduler control after a callback catches or replaces Python unwind."""

import gc
import weakref

from amal.eda.systemc import sc_core as sc


timeline = []
destroyed_cleanup_errors = []
cleanup_error_refs = []


class CleanupFailure(BaseException):
    """Track a Python cleanup exception that races with native control flow."""

    __slots__ = ("label", "__weakref__")

    def __init__(self, label):
        """Record the reset/kill invocation that creates this exception.

        Args:
            label: Stable invocation label retained for destructor accounting.
        """
        self.label = label

    def __del__(self):
        """Count final release after native reset/kill dispatch discards the error."""
        destroyed_cleanup_errors.append(self.label)


class CallbackOwner:
    """Record wait starts, unwind cleanup, and Python exception release."""

    def __init__(self, name, *, suppress_unwind):
        """Initialize one independently retained registered callback target.

        Args:
            name: Stable label for timeline assertions.
            suppress_unwind: Whether the callback catches and returns from BaseException.
        """
        self.name = name
        self.suppress_unwind = suppress_unwind
        self.starts = []
        self.unwinds = []
        self.cleanup_times = []

    def run(self):
        """Block until reset/kill, then suppress or replace Python unwind.

        Returns:
            None when the Python callback reaches its expected interrupted exit.
        """
        invocation = len(self.starts) + 1
        now = int(sc.sc_time_stamp().value())
        self.starts.append((invocation, now))
        timeline.append((self.name, "start", invocation, now))
        interrupted = False
        try:
            sc.wait(100, sc.SC_NS)
        except BaseException as unwind:
            interrupted = True
            if self.suppress_unwind:
                self.unwinds.append((invocation, int(sc.sc_time_stamp().value()), type(unwind).__name__))
                timeline.append((self.name, "suppressed", invocation, int(sc.sc_time_stamp().value())))
                return
            raise
        else:
            raise AssertionError("long wait completed before controller reset/kill")
        finally:
            if not self.suppress_unwind and interrupted:
                cleanup_time = int(sc.sc_time_stamp().value())
                self.cleanup_times.append(cleanup_time)
                timeline.append((self.name, "finally", invocation, cleanup_time))
                failure = CleanupFailure(f"{self.name}-{invocation}")
                cleanup_error_refs.append(weakref.ref(failure))
                raise failure


class Model(sc.sc_module):
    """Own process callbacks and reset/kill handles for one fresh kernel."""

    def __init__(self):
        """Register two waiting callbacks and one native process controller."""
        super().__init__(sc.sc_module_name("section1_lifecycle_edges"))
        self.suppressor = CallbackOwner("suppressor", suppress_unwind=True)
        self.raiser = CallbackOwner("raiser", suppress_unwind=False)
        self.sc_thread("suppressor", self.suppressor.run)
        self.suppress_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("raiser", self.raiser.run)
        self.raise_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("controller", self.control)

    def control(self):
        """Reset both waiters at 1 ns, kill them at 2 ns, and assert progress."""
        sc.wait(1, sc.SC_NS)
        timeline.append(("controller", "reset", int(sc.sc_time_stamp().value())))
        self.suppress_handle.reset()
        self.raise_handle.reset()
        self._yield_until(lambda: len(self.suppressor.starts) == 2 and len(self.raiser.starts) == 2)
        assert int(sc.sc_time_stamp().value()) == 1000

        sc.wait(1, sc.SC_NS)
        timeline.append(("controller", "kill", int(sc.sc_time_stamp().value())))
        self.suppress_handle.kill()
        self.raise_handle.kill()
        self._yield_until(lambda: self.suppress_handle.terminated() and self.raise_handle.terminated())
        assert int(sc.sc_time_stamp().value()) == 2000
        sc.sc_stop()

    @staticmethod
    def _yield_until(predicate):
        """Advance delta cycles until reset restart or kill termination completes.

        Args:
            predicate: Zero-argument completion condition.

        Raises:
            AssertionError: The requested process state does not appear in 16 deltas.
        """
        for _ in range(16):
            if predicate():
                return
            sc.wait(sc.SC_ZERO_TIME)
        raise AssertionError("process control did not finish within 16 delta cycles")


model = Model()
suppressor_ref = weakref.ref(model.suppressor)
raiser_ref = weakref.ref(model.raiser)
sc.sc_start()

assert model.suppressor.starts == [(1, 0), (2, 1000)], model.suppressor.starts
assert model.suppressor.unwinds == [
    (1, 1000, "_ProcessUnwind"),
    (2, 2000, "_ProcessUnwind"),
], model.suppressor.unwinds
assert model.raiser.starts == [(1, 0), (2, 1000)], model.raiser.starts
assert model.raiser.cleanup_times == [1000, 2000], model.raiser.cleanup_times
assert model.suppress_handle.terminated() and model.raise_handle.terminated()
assert timeline.index(("controller", "reset", 1000)) < timeline.index(("raiser", "finally", 1, 1000))
assert timeline.index(("controller", "kill", 2000)) < timeline.index(("raiser", "finally", 2, 2000))

# The module must retain callbacks until its native process host is explicitly gone.
assert suppressor_ref() is model.suppressor and raiser_ref() is model.raiser
model.suppressor = None
model.raiser = None
gc.collect()
assert suppressor_ref() is not None and raiser_ref() is not None
model.__destruct__()
del model
gc.collect()
assert suppressor_ref() is None and raiser_ref() is None
assert len(cleanup_error_refs) == 2
assert all(reference() is None for reference in cleanup_error_refs)
assert sorted(destroyed_cleanup_errors) == ["raiser-1", "raiser-2"], destroyed_cleanup_errors

print("SECTION1_LIFECYCLE reset_ns=1000 kill_ns=2000 suppressed=2 raised=2")
print("SECTION1_LIFECYCLE_RELEASE callbacks=2 cleanup_errors=2")
