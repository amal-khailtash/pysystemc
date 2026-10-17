"""Validate Python overrides of the source-defined event-queue notification interface."""

from amal.eda.systemc import sc_core as sc


class EventQueue:
    """Delegate scheduling to an owned native queue while implementing its interface."""

    def __init__(self, name):
        """Retain native queue and borrowed event storage for simulation lifetime."""
        self.m_queue = sc.sc_event_queue(sc.sc_module_name(name))
        self.calls = []

    def notify(self, delay):
        """Schedule the native time normalized from either notification overload."""
        self.calls.append(str(delay))
        self.m_queue.notify(delay)

    def default_event(self):
        """Return the borrowed native event retained by the owned queue."""
        return self.m_queue.default_event()

    def cancel_all(self):
        """Cancel every pending notification without replacing event storage."""
        self.calls.append("cancel")
        self.m_queue.cancel_all()


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
queue = EventQueue("events")
port = sc.sc_port[sc.sc_event_queue_if]("port")
port(queue)
records = []


def observe():
    """Record trigger times and deltas to distinguish duplicate notifications."""
    records.append((str(sc.sc_time_stamp()), int(sc.sc_delta_count())))


model.sc_method("observe", observe)
model.sensitive(port.get_interface().default_event())
model.dont_initialize()
del module_name


def stimulus():
    """Exercise both overloads, same-time duplicates and cancellation via the port."""
    interface = port.get_interface()
    interface.notify(1.0, sc.SC_NS)
    interface.notify(sc.sc_time(1, sc.SC_NS))
    interface.notify(sc.sc_time(5, sc.SC_NS))
    sc.wait(2, sc.SC_NS)
    interface.cancel_all()
    interface.notify(sc.sc_time(1, sc.SC_NS))


handle = sc.sc_spawn(stimulus, "stimulus")
sc.sc_start(6, sc.SC_NS)
assert [time for time, delta in records] == ["1 ns", "1 ns", "3 ns"], records
assert records[1][1] > records[0][1], records
assert queue.calls == ["1 ns", "1 ns", "5 ns", "cancel", "1 ns"], queue.calls
assert handle.terminated()
sc.sc_stop()
print("NATIVE_EVENT_QUEUE_INTERFACE_OK")
