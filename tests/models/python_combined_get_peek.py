"""Validate full get-peek inheritance through native combined and base views."""

import ctypes

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm


class Queue:
    """Share samples between consuming get and nonconsuming peek callbacks."""

    def __init__(self):
        """Initialize publication event, samples and callback history."""
        self.values = []
        self.ready = sc.sc_event("ready")
        self.calls = []

    def get(self):
        """Wait for publication and consume one sample."""
        self.calls.append("get")
        while not self.values:
            sc.wait(self.ready)
        return self.values.pop(0)

    def peek(self):
        """Wait for publication without consuming the sample."""
        self.calls.append("peek")
        while not self.values:
            sc.wait(self.ready)
        return self.values[0]

    def nb_get(self):
        """Return immediate success and consume only available samples."""
        self.calls.append("nb_get")
        return (True, self.values.pop(0)) if self.values else (False, 0)

    def nb_peek(self):
        """Return immediate sample availability without consumption."""
        self.calls.append("nb_peek")
        return (True, self.values[0]) if self.values else (False, 0)

    def nb_can_get(self):
        """Report immediate get availability."""
        return bool(self.values)

    def nb_can_peek(self):
        """Report immediate peek availability."""
        return bool(self.values)


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
queue = Queue()
port = sc.sc_port[tlm.tlm_get_peek_if[int]]("combined")
port.bind(queue)
blocking = sc.sc_port[tlm.tlm_blocking_get_peek_if[int]]("blocking")
nonblocking = sc.sc_port[tlm.tlm_nonblocking_get_peek_if[int]]("nonblocking")
blocking.bind(port.get_interface())
nonblocking.bind(port.get_interface())
del module_name
records = []


def consumer():
    """Check shared state and overload dispatch through each native base view."""
    interface = port.get_interface()
    value = ctypes.c_int(-1)
    assert not nonblocking.get_interface().nb_peek(value) and value.value == -1
    assert blocking.get_interface().peek() == 17
    interface.peek(value)
    assert value.value == 17 and queue.values == [17]
    assert blocking.get_interface().get() == 17
    interface.get(value)
    assert value.value == 23
    sc.wait(interface.ok_to_get() & interface.ok_to_peek())
    assert interface.nb_can_get() and interface.nb_can_peek()
    assert nonblocking.get_interface().nb_peek(value) and value.value == 41
    assert queue.values == [41]
    assert nonblocking.get_interface().nb_get(value) and value.value == 41
    assert not interface.nb_get(value) and value.value == 41
    records.append(str(sc.sc_time_stamp()))


def producer():
    """Publish consecutive samples with both native readiness notifications."""
    for value in (17, 23, 41):
        sc.wait(1, sc.SC_NS)
        queue.values.append(value)
        queue.ready.notify(sc.SC_ZERO_TIME)
        port.get_interface().notify_ready()


handles = [sc.sc_spawn(consumer, "consumer"), sc.sc_spawn(producer, "producer")]
sc.sc_start(4, sc.SC_NS)
assert records == ["3 ns"]
assert queue.values == []
assert queue.calls == ["nb_peek", "peek", "peek", "get", "get", "nb_peek", "nb_get", "nb_get"]
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("PYTHON_COMBINED_GET_PEEK_OK")
