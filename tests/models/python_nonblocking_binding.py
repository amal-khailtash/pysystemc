"""Validate combined TLM-1 get/peek dispatch through combined and base ports."""

import ctypes

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm


class Queue:
    """Retain samples and adapter-owned readiness events throughout simulation."""

    def __init__(self):
        """Initialize the native module and sample queue."""
        self.values = []
        self.calls = []

    def nb_get(self):
        """Consume one available sample and return native success/value pair."""
        self.calls.append("get")
        return (True, self.values.pop(0)) if self.values else (False, 0)

    def nb_peek(self):
        """Return an available sample without consuming it."""
        self.calls.append("peek")
        return (True, self.values[0]) if self.values else (False, 0)

    def nb_can_get(self):
        """Report availability without modifying the queue."""
        return bool(self.values)

    def nb_can_peek(self):
        """Report availability without modifying the queue."""
        return bool(self.values)


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
channel = Queue()
combined = sc.sc_port[tlm.tlm_nonblocking_get_peek_if[int]]("combined")
get_port = sc.sc_port[tlm.tlm_nonblocking_get_if[int]]("get_port")
peek_port = sc.sc_port[tlm.tlm_nonblocking_peek_if[int]]("peek_port")
for port in (combined, get_port, peek_port):
    port.bind(channel)
del module_name
records = []


def consumer():
    """Check failed outputs, repeated peeks, base dispatch and one consumption."""
    interface = combined.get_interface()
    value = ctypes.c_int(-1)
    assert not interface.nb_can_get() and not interface.nb_can_peek()
    assert not interface.nb_get(value) and value.value == -1
    assert not interface.nb_peek(value) and value.value == -1
    sc.wait(interface.ok_to_get() & interface.ok_to_peek())
    assert interface.nb_can_get() and interface.nb_can_peek()
    assert interface.nb_peek(value) and value.value == 23
    assert peek_port.get_interface().nb_peek(value) and value.value == 23
    assert channel.values == [23]
    assert get_port.get_interface().nb_get(value) and value.value == 23
    assert not interface.nb_peek(value) and value.value == 23
    assert not interface.nb_get(value) and value.value == 23
    assert not interface.nb_can_get() and not interface.nb_can_peek()
    records.append(str(sc.sc_time_stamp()))


def producer():
    """Schedule both readiness events for2ns after publishing one sample at1ns."""
    sc.wait(1, sc.SC_NS)
    channel.values.append(23)
    combined.get_interface().notify_ready(sc.sc_time(1, sc.SC_NS))


handles = [sc.sc_spawn(consumer, "consumer"), sc.sc_spawn(producer, "producer")]
sc.sc_start(3, sc.SC_NS)
assert records == ["2 ns"], records
assert channel.values == []
assert channel.calls == ["get", "peek", "peek", "peek", "get", "peek", "get"]
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("PYTHON_NONBLOCKING_BINDING_OK")
