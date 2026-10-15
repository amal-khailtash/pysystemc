"""Validate combined TLM-1 get/peek dispatch through combined and base ports."""

import ctypes

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm

import cppyy


class Queue(sc.channel_interface(tlm.tlm_nonblocking_get_peek_if[int])):
    """Retain samples and adapter-owned readiness events throughout simulation."""

    def __init__(self, name):
        """Initialize the native module and sample queue."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.values = []
        self.calls = []

    def get_value(self):
        """Consume one available sample and return native success/value pair."""
        self.calls.append("get")
        pair = cppyy.gbl.std.pair[bool, int]
        return pair(True, self.values.pop(0)) if self.values else pair(False, 0)

    def peek_value(self):
        """Return an available sample without consuming it."""
        self.calls.append("peek")
        pair = cppyy.gbl.std.pair[bool, int]
        return pair(True, self.values[0]) if self.values else pair(False, 0)

    def can_get(self):
        """Report availability without modifying the queue."""
        return bool(self.values)

    def can_peek(self):
        """Report availability without modifying the queue."""
        return bool(self.values)


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
channel = Queue("queue")
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
    channel.notify_ready(sc.sc_time(1, sc.SC_NS))


handles = [sc.sc_spawn(consumer, "consumer"), sc.sc_spawn(producer, "producer")]
sc.sc_start(3, sc.SC_NS)
assert records == ["2 ns"], records
assert channel.values == []
assert channel.calls == ["get", "peek", "peek", "peek", "get", "peek", "get"]
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("NATIVE_TLM1_NONBLOCKING_GET_PEEK_OK")
