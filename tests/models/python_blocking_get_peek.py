"""Validate structural blocking get-peek value and output-reference overloads."""

import ctypes

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm


class Queue:
    """Retain samples for peek and consume them for get after service waits."""

    def __init__(self):
        """Initialize two samples and ordered callback records."""
        self.values = [17, 23]
        self.calls = []

    def get(self):
        """Wait1ns before consuming one sample."""
        sc.wait(1, sc.SC_NS)
        self.calls.append("get")
        return self.values.pop(0)

    def peek(self):
        """Wait1ns before returning a nonconsuming sample copy."""
        sc.wait(1, sc.SC_NS)
        self.calls.append("peek")
        return self.values[0]


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
queue = Queue()
port = sc.sc_port[tlm.tlm_blocking_get_peek_if[int]]("combined")
port.bind(queue)
del module_name
records = []


def consumer():
    """Check native overload values, callback waits and sample ownership."""
    interface = port.get_interface()
    value = ctypes.c_int(-1)
    assert interface.peek() == 17
    interface.peek(value)
    assert value.value == 17 and queue.values == [17, 23]
    assert interface.get() == 17
    interface.get(value)
    assert value.value == 23
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(consumer, "consumer")
sc.sc_start(5, sc.SC_NS)
assert records == ["4 ns"]
assert queue.values == []
assert queue.calls == ["peek", "peek", "get", "get"]
assert handle.terminated()
sc.sc_stop()
print("PYTHON_BLOCKING_GET_PEEK_OK")
