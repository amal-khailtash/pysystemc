"""Compare process-handle ownership, metadata and event-driven controls to native."""

from amal.eda.systemc import sc_core as sc
import cppyy


records = []


def conversions(handle):
    """Call each public native pointer conversion directly through cppyy reflection."""
    pointers = [
        getattr(handle, "operator sc_core::sc_" + kind + "_process *")() for kind in ("method", "thread", "cthread")
    ]
    return sum(int(bool(pointer)) * weight for pointer, weight in zip(pointers, (1, 2, 4)))


def record(label, *values):
    """Store deterministic values without pointer-address differences."""
    records.append("HANDLE_REFERENCE " + label + " " + " ".join(str(v) for v in values))


empty = sc.sc_process_handle()
assert not empty.valid() and not empty.dynamic() and not empty.terminated()
assert empty.name() == "" and empty.basename() == "" and empty.proc_kind() == sc.SC_NO_PROC_
assert not empty.get_parent_object() and not empty.get_process_object()
assert len(empty.get_child_objects()) == len(empty.get_child_events()) == 0
assert empty.dump_state() == "NO TARGET"
plain = sc.sc_semaphore("plain", 1)
assert not sc.sc_process_handle(plain).valid()
record("empty", 0, 0, 0)
name = sc.sc_module_name("handles")
model = sc.sc_module(name)
event = sc.sc_event("tick")
visits = []


def method():
    """Check borrowed metadata during dispatch and record exact simulation time."""
    h = sc.sc_get_current_process_handle()
    assert cppyy.addressof(getattr(h, "operator sc_core::sc_process_b *")()) == cppyy.addressof(
        sc.sc_get_curr_process_handle()
    )
    assert h.proc_kind() == sc.SC_METHOD_PROC_ and not h.dynamic()
    assert h.basename() == "method" and h.get_parent_object().name() == "handles"
    assert h.get_process_object().name() == "handles.method"
    assert not h.is_unwinding()
    visits.append(int(sc.sc_time_stamp().value()))
    record("visit", len(visits), str(sc.sc_time_stamp()))


model.sc_method("method", method)
model.sensitive(event)
original = sc.sc_get_last_created_process_handle()
copy = sc.sc_process_handle(original)
from_object = sc.sc_process_handle(original.get_process_object())
raw_pointer = getattr(original, "operator sc_core::sc_process_b *")()
from_raw = sc.sc_process_handle(raw_pointer)
assert all(h.valid() and h.name() == "handles.method" for h in (copy, from_object, from_raw))
assert conversions(original) == 1
assigned = sc.sc_process_handle()
assigned.__assign__(copy)
assert assigned.name() == original.name()
assigned.__assign__(assigned)
assigned.swap(empty)
assert empty.valid() and not assigned.valid()
assigned.swap(empty)
assert assigned.valid() and not empty.valid()
copy.__destruct__()
assert original.valid() and assigned.valid()
assert len(original.get_child_objects()) == 0
# reset/termination events are lazily owned by the retained process.
assert cppyy.addressof(original.reset_event()) == cppyy.addressof(original.reset_event())
assert cppyy.addressof(original.terminated_event()) == cppyy.addressof(original.terminated_event())
record("owned", int(original.valid()), int(original.dynamic()), original.name(), int(original.proc_kind()))
clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
clock_port = sc.sc_in[bool]("clock_port")
clock_port.bind(clock)


def clocked():
    """Verify cthread conversion includes its thread base, then wait one cycle."""
    handle = sc.sc_get_current_process_handle()
    assert handle.proc_kind() == sc.SC_CTHREAD_PROC_
    assert conversions(handle) == 6
    sc.wait()
    record("clocked", int(handle.proc_kind()), str(sc.sc_time_stamp()))


model.sc_cthread("clocked", clocked, clock_port.pos())
del name


def control():
    """Disable/drop, suspend/retain and resume a pending event-driven invocation."""
    sc.wait(1, sc.SC_NS)
    dynamic = sc.sc_spawn(lambda: None, "runtime_child")
    assert dynamic.dynamic() and dynamic.get_parent_object().name() == "controller"
    current = sc.sc_get_current_process_handle()
    owned_event = sc.sc_event("child_event")
    assert owned_event.basename() == "child_event"
    assert len(current.get_child_objects()) == 1 and len(current.get_child_events()) == 1
    record("dynamic", int(dynamic.dynamic()), dynamic.get_parent_object().name())
    original.disable()
    event.notify(sc.SC_ZERO_TIME)
    sc.wait(1, sc.SC_NS)
    assert len(visits) == 1
    original.enable()
    event.notify(sc.SC_ZERO_TIME)
    sc.wait(1, sc.SC_NS)
    assert len(visits) == 2
    original.suspend()
    event.notify(sc.SC_ZERO_TIME)
    sc.wait(1, sc.SC_NS)
    assert len(visits) == 2
    original.resume()
    sc.wait(1, sc.SC_NS)
    assert len(visits) == 3
    # Resuming without a retained invocation does not execute again.
    original.resume()
    sc.wait(1, sc.SC_NS)
    assert len(visits) == 3
    record("controls", len(visits), str(sc.sc_time_stamp()))
    previous = sc.sc_report_handler.set_actions(sc.SC_WARNING, sc.SC_THROW)
    operations = (
        "disable",
        "enable",
        "suspend",
        "resume",
        "sync_reset_on",
        "sync_reset_off",
        "reset",
        "kill",
        "reset_event",
        "terminated_event",
        "is_unwinding",
    )
    for operation in operations:
        try:
            getattr(empty, operation)()
        except sc.sc_report as error:
            assert "empty process handle" in error.get_msg_type().lower()
        else:
            raise AssertionError("empty handle did not report: " + operation)
    sc.sc_report_handler.set_actions(sc.SC_WARNING, previous)
    record("invalid", len(operations))


controller = sc.sc_spawn(control, "controller")
assert not controller.dynamic() and controller.proc_kind() == sc.SC_THREAD_PROC_
assert conversions(controller) == 2
retained = sc.sc_process_handle(controller)
original.suspend()
original.resume()
sc.sc_start(7, sc.SC_NS)
assert retained.valid() and retained.terminated() and not original.terminated()
original.suspend()
original.resume()
assert retained.name() == "controller" and not retained.is_unwinding()
controller.__destruct__()
assert retained.valid() and retained.terminated()
record("retained", int(retained.valid()), int(retained.terminated()), retained.name())
sc.sc_stop()
for line in records:
    print(line)
