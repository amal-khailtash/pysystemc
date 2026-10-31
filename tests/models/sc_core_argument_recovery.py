"""Exercise public API argument errors followed by native recovery calls."""

from amal.eda.systemc import sc_core as sc
import cppyy


cppyy.cppdef(
    """
    /** @brief Store the value received by the recovered bound native process. */
    int argument_recovery_value = 0;
    /**
     * @brief Record a value from a synchronously invoked native call.
     *
     * This function does not wait or call Python. The Python entry invokes it
     * with the GIL held.
     */
    void record_argument_recovery(int value) { argument_recovery_value = value; }
    """
)


def expect_type_error(operation, expected_text):
    """Require an operation to reject invalid arguments with TypeError."""
    try:
        operation()
    except TypeError as error:
        assert expected_text in str(error), (expected_text, str(error))
    else:
        raise AssertionError(f"expected TypeError containing {expected_text!r}")


options = sc.sc_spawn_options()
callbacks = []


def spawn_recovery(label):
    """Register a process that records its execution and simulation time."""

    def callback():
        """Record this recovered process execution."""
        callbacks.append((label, sc.sc_time_stamp().value()))

    return sc.sc_spawn(callback, f"argument_recovery_{label}")


expect_type_error(lambda: sc.sc_spawn(41), "callback must be callable")
spawn_recovery("invalid_callable")

expect_type_error(
    lambda: sc.sc_spawn(lambda: None, "too_many", options, "extra"),
    "optional name and options",
)
spawn_recovery("too_many")

expect_type_error(
    lambda: sc.sc_spawn(lambda: None, "positional", name="keyword"),
    "name supplied twice",
)
spawn_recovery("duplicate_name")

expect_type_error(
    lambda: sc.sc_spawn(lambda: None, "duplicate_options", options, options=options),
    "options supplied twice",
)
spawn_recovery("duplicate_options")

cppyy.gbl.argument_recovery_value = 0
expect_type_error(
    lambda: sc.sc_bind(cppyy.gbl.record_argument_recovery, object()),
    "native values or numeric/string scalars",
)
bound_recovery = sc.sc_bind(cppyy.gbl.record_argument_recovery, 41)
sc.sc_spawn(bound_recovery, "argument_recovery_bound")

module_name = sc.sc_module_name("argument_recovery")
module = sc.sc_module(module_name)
event = sc.sc_event("argument_recovery_event")
event_times = []


def observe_event():
    """Record the real event-trigger timestamp after valid recovery."""
    assert event.triggered()
    event_times.append(sc.sc_time_stamp().value())


def notify_recovery():
    """Reject a wrong notify_delayed arity and then schedule a valid event."""
    expect_type_error(
        lambda: event.notify_delayed(1, sc.SC_NS, sc.SC_NS),
        "zero, one or two arguments",
    )
    event.notify_delayed(1, sc.SC_NS)
    sc.wait(2, sc.SC_NS)
    sc.sc_stop()


module.sc_method("argument_recovery_observer", observe_event)
module.sensitive(event)
module.dont_initialize()
sc.sc_spawn(notify_recovery, "argument_recovery_notifier")
del module_name

sc.sc_start()

assert len(callbacks) == 4, callbacks
assert {label for label, _ in callbacks} == {
    "invalid_callable",
    "too_many",
    "duplicate_name",
    "duplicate_options",
}, callbacks
assert all(time == 0 for _, time in callbacks), callbacks
assert event_times == [1000], event_times
assert sc.sc_time_stamp().value() == 2000, sc.sc_time_stamp().value()
assert cppyy.gbl.argument_recovery_value == 41, cppyy.gbl.argument_recovery_value
print("ARGUMENT_RECOVERY_OK python_callbacks=4 event=1000 bind=41")
