"""Compare Python process-handle restriction reports with native SystemC."""

from amal.eda.systemc import sc_core as sc
import cppyy


def capture(label, operation):
    """Record a caught native report's type, message and severity."""
    try:
        operation()
    except sc.sc_report as report:
        print(f"PROCESS_ERROR {label} {report.get_msg_type()} | {report.get_msg()} | {int(report.get_severity())}")
    else:
        raise AssertionError(f"missing native report: {label}")


name = sc.sc_module_name("process_error_matrix")
model = sc.sc_module(name)
event = sc.sc_event("method_event")
reset = sc.sc_signal[bool]("reset")


def method():
    """Stay available as a statically sensitive process target."""


def reset_method():
    """Stay available as a reset-sensitive process target."""


model.sc_method("method", method)
model.sensitive(event)
method_handle = sc.sc_get_last_created_process_handle()
model.sc_method("reset_method", reset_method)
model.sensitive(event)
model.reset_signal_is(reset, True)
reset_method_handle = sc.sc_get_last_created_process_handle()


def thread():
    """Keep a live thread handle while its pre-simulation restriction is checked."""
    sc.wait(5, sc.SC_NS)


model.sc_thread("thread", thread)
thread_handle = sc.sc_get_last_created_process_handle()


def completed():
    """Finish during initialization so throw_it sees a terminal target."""


model.sc_thread("completed", completed)
completed_handle = sc.sc_get_last_created_process_handle()
exception = cppyy.gbl.std.runtime_error("unused native exception")
previous_warning = sc.sc_report_handler.set_actions(sc.SC_WARNING, sc.SC_THROW)
previous_error = sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_THROW)
capture("throw_before_simulation", lambda: thread_handle.throw_it(exception))


def controller():
    """Check method, empty-handle and suspend/reset corner diagnostics."""
    sc.wait(1, sc.SC_NS)
    empty = sc.sc_process_handle()
    capture("empty_throw", lambda: empty.throw_it(exception))
    capture("method_throw", lambda: method_handle.throw_it(exception))
    capture("throw_terminated_thread", lambda: completed_handle.throw_it(exception))
    capture("suspend_reset_method", reset_method_handle.suspend)

    method_handle.disable()
    method_handle.suspend()
    capture("resume_disabled_suspended_method", method_handle.resume)
    method_handle.enable()
    sc.sc_stop()


model.sc_thread("controller", controller)
del name
sc.sc_start(sc.SC_ZERO_TIME)
capture("throw_while_paused", lambda: thread_handle.throw_it(exception))
sc.sc_start(3, sc.SC_NS)
capture("throw_after_stop", lambda: thread_handle.throw_it(exception))
sc.sc_report_handler.set_actions(sc.SC_WARNING, previous_warning)
sc.sc_report_handler.set_actions(sc.SC_ERROR, previous_error)

# The Python-facing adapter rejects non-native exception values before cppyy.
try:
    thread_handle.throw_it(object())
except TypeError as error:
    assert str(error) == "throw_it requires a native C++ exception value"
    print("PROCESS_ERROR python_exception_type TypeError | " + str(error))
else:
    raise AssertionError("throw_it accepted a Python object")
