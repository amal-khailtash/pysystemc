"""Validate nonconsuming blocking/nonblocking peek through native combined ports."""

import ctypes

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm


class Sample:
    """Retain one sample across blocking and nonblocking peek calls."""

    def __init__(self):
        """Initialize empty storage, publication event and callback records."""
        self.values = []
        self.ready = sc.sc_event("ready")
        self.calls = []

    def peek(self):
        """Wait for publication and return without consuming the sample."""
        self.calls.append("peek")
        while not self.values:
            sc.wait(self.ready)
        return self.values[0]

    def nb_peek(self):
        """Return immediate availability and sample without waiting."""
        self.calls.append("nb_peek")
        return (True, self.values[0]) if self.values else (False, 0)

    def nb_can_peek(self):
        """Report publication without changing storage."""
        return bool(self.values)


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
sample = Sample()
port = sc.sc_port[tlm.tlm_peek_if[int]]("combined")
port.bind(sample)
del module_name
records = []


def consumer():
    """Check failed output, blocking overloads and repeated readiness access."""
    interface = port.get_interface()
    value = ctypes.c_int(-1)
    assert not interface.nb_can_peek()
    assert not interface.nb_peek(value) and value.value == -1
    assert interface.peek() == 23
    interface.peek(value)
    assert value.value == 23
    sc.wait(interface.ok_to_peek())
    assert interface.nb_can_peek()
    assert interface.nb_peek(value) and value.value == 23
    assert interface.nb_peek(value) and value.value == 23
    assert sample.values == [23]
    records.append(str(sc.sc_time_stamp()))


def producer():
    """Publish at1ns and schedule native readiness for2ns."""
    sc.wait(1, sc.SC_NS)
    sample.values.append(23)
    sample.ready.notify(sc.SC_ZERO_TIME)
    port.get_interface().notify_ready(sc.sc_time(1, sc.SC_NS))


handles = [sc.sc_spawn(consumer, "consumer"), sc.sc_spawn(producer, "producer")]
sc.sc_start(3, sc.SC_NS)
assert records == ["2 ns"]
assert sample.values == [23]
assert sample.calls == ["nb_peek", "peek", "peek", "nb_peek", "nb_peek"]
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("PYTHON_COMBINED_PEEK_OK")
