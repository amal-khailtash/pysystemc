"""Validate TLM-1 transport and peek value/reference overloads through native ports."""

import ctypes

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm


class Transport(sc.channel_interface(tlm.tlm_transport_if[int, int])):
    """Compute a response after a native process wait."""

    def __init__(self, name):
        """Initialize channel identity and request records."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.requests = []

    def transport_value(self, request):
        """Wait one nanosecond and return twice the request value."""
        sc.wait(1, sc.SC_NS)
        self.requests.append(int(request))
        return int(request) * 2


class Peek(sc.channel_interface(tlm.tlm_blocking_peek_if[int])):
    """Return a retained sample without consuming it."""

    def __init__(self, name):
        """Initialize channel identity and sample state."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.value = 23

    def peek_value(self):
        """Return the current sample without changing state or simulation time."""
        return self.value


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
transport = Transport("transport")
peek = Peek("peek")
transport_port = sc.sc_port[tlm.tlm_transport_if[int, int]]("transport_port")
peek_port = sc.sc_port[tlm.tlm_blocking_peek_if[int]]("peek_port")
transport_port.bind(transport)
peek_port.bind(peek)
del module_name
type(transport_port.get_interface()).transport.__release_gil__ = True
records = []


def process():
    """Check response values and native reference-output overload assignment."""
    interface = transport_port.get_interface()
    assert interface.transport(17) == 34
    response = ctypes.c_int(-1)
    interface.transport(19, response)
    assert response.value == 38, response.value
    peek_interface = peek_port.get_interface()
    assert peek_interface.peek() == 23
    value = ctypes.c_int(-1)
    peek_interface.peek(value)
    assert value.value == 23, value.value
    assert peek.value == 23
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(process, "process")
sc.sc_start(3, sc.SC_NS)
assert transport.requests == [17, 19]
assert records == ["2 ns"], records
assert handle.terminated()
sc.sc_stop()
print("NATIVE_TLM1_TRANSPORT_PEEK_OK")
