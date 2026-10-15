"""Validate shared blocking/nonblocking get state through native interface views."""

import ctypes

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm


class Queue:
    """Implement blocking and immediate access to one Python queue."""

    def __init__(self):
        """Initialize queue state and callback records."""
        self.values = []
        self.calls = []
        self.ready = sc.sc_event("ready")

    def get(self):
        """Wait for a sample, then consume exactly one value."""
        self.calls.append("get")
        while not self.values:
            sc.wait(self.ready)
        return self.values.pop(0)

    def nb_get(self):
        """Return a success/value tuple without waiting."""
        self.calls.append("nb_get")
        return (True, self.values.pop(0)) if self.values else (False, 0)

    def nb_can_get(self):
        """Report immediate availability without consuming state."""
        return bool(self.values)


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
queue = Queue()
port = sc.sc_port[tlm.tlm_get_if[int]]("combined")
port.bind(queue)
del module_name
records = []


def consumer():
    """Check blocking overloads, readiness and failed output preservation."""
    interface = port.get_interface()
    value = ctypes.c_int(-1)
    assert not interface.nb_get(value) and value.value == -1
    assert interface.get() == 17
    interface.get(value)
    assert value.value == 23
    sc.wait(interface.ok_to_get())
    assert interface.nb_can_get()
    assert interface.nb_get(value) and value.value == 41
    assert not interface.nb_get(value) and value.value == 41
    records.append(str(sc.sc_time_stamp()))


def producer():
    """Publish two blocking samples then notify native readiness for a third."""
    for value in (17, 23, 41):
        sc.wait(1, sc.SC_NS)
        queue.values.append(value)
        queue.ready.notify(sc.SC_ZERO_TIME)
        port.get_interface().notify_ready()


handles = [sc.sc_spawn(consumer, "consumer"), sc.sc_spawn(producer, "producer")]
sc.sc_start(4, sc.SC_NS)
assert records == ["3 ns"]
assert queue.values == []
assert queue.calls == ["nb_get", "get", "get", "nb_get", "nb_get"]
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("PYTHON_COMBINED_GET_OK")
