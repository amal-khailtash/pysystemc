"""Validate nonblocking TLM-1 get output references and native readiness events."""

import ctypes

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm

import cppyy


class Get(sc.channel_interface(tlm.tlm_nonblocking_get_if[int])):
    """Own Python sample state behind a native reference-assignment adapter."""

    def __init__(self, name):
        """Initialize identity and an empty sample queue."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.values = []

    def get_value(self):
        """Return success and one consumed value, or failure without consumption."""
        result = cppyy.gbl.std.pair[bool, int]
        return result(True, self.values.pop(0)) if self.values else result(False, 0)

    def can_get(self):
        """Report whether a sample is available without consuming it."""
        return bool(self.values)


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
channel = Get("get")
port = sc.sc_port[tlm.tlm_nonblocking_get_if[int]]("port")
port.bind(channel)
del module_name
records = []


def consumer():
    """Check unchanged failure output, event wakeup and exact sample consumption."""
    interface = port.get_interface()
    value = ctypes.c_int(-1)
    assert not interface.nb_can_get()
    assert not interface.nb_get(value)
    assert value.value == -1
    sc.wait(interface.ok_to_get())
    assert interface.nb_can_get()
    assert interface.nb_get(value)
    assert value.value == 23
    assert not interface.nb_get(value)
    assert value.value == 23
    records.append(str(sc.sc_time_stamp()))


def producer():
    """Make a sample available at2ns and notify the adapter-owned event."""
    sc.wait(2, sc.SC_NS)
    channel.values.append(23)
    channel.notify_ready()


handles = [sc.sc_spawn(consumer, "consumer"), sc.sc_spawn(producer, "producer")]
sc.sc_start(3, sc.SC_NS)
assert channel.values == []
assert records == ["2 ns"], records
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("NATIVE_TLM1_NONBLOCKING_GET_OK")
