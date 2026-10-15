"""Probe ordinary Protocol implementations through native TLM ports."""

import ctypes
import gc
from typing import Protocol
import weakref

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm

from interface_prototype import implement


class TransportInterface(Protocol):
    """Python value-return spelling of the native transport contract."""

    def transport(self, request: int) -> int:
        """Return a response; execution may wait in a native thread process."""
        ...


class Transport:
    """Implement the protocol without cppyy or Protocol inheritance."""

    def __init__(self):
        """Initialize the observed requests."""
        self.requests = []

    def transport(self, request):
        """Wait1ns then double the request, preserving callback exceptions."""
        if request < 0:
            raise ValueError("negative request")
        sc.wait(1, sc.SC_NS)
        self.requests.append(int(request))
        return request * 2


class Peek:
    """Provide an ordinary nonconsuming sample implementation."""

    def peek(self):
        """Return the sample without waiting."""
        return 23


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
implementation: TransportInterface = Transport()
owner = weakref.ref(implementation)
transport = implement(tlm.tlm_transport_if[int, int], implementation, "transport")
peek = implement(tlm.tlm_blocking_peek_if[int], Peek(), "peek")
transport_port = sc.sc_port[tlm.tlm_transport_if[int, int]]("transport_port")
peek_port = sc.sc_port[tlm.tlm_blocking_peek_if[int]]("peek_port")
transport_port.bind(transport)
peek_port.bind(peek)
del module_name, implementation
gc.collect()
assert owner() is not None
try:
    implement(tlm.tlm_blocking_get_if[int], object(), "missing")
except TypeError as error:
    assert "requires callable get" in str(error)
else:
    raise AssertionError("missing method accepted")
type(transport_port.get_interface()).transport.__release_gil__ = True
records = []


def process():
    """Assert blocking dispatch, both native overloads and Python exception identity."""
    interface = transport_port.get_interface()
    assert interface.transport(17) == 34
    response = ctypes.c_int(-1)
    interface.transport(19, response)
    assert response.value == 38
    try:
        interface.transport(-1)
    except ValueError as error:
        assert "negative request" in str(error)
    else:
        raise AssertionError("callback exception lost")
    sample = ctypes.c_int(-1)
    peek_interface = peek_port.get_interface()
    assert peek_interface.peek() == 23
    peek_interface.peek(sample)
    assert sample.value == 23
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(process, "process")
sc.sc_start(3, sc.SC_NS)
assert records == ["2 ns"], records
assert owner().requests == [17, 19]
assert handle.terminated()
sc.sc_stop()
print("PROTOCOL_COMPOSITION_OK")
