"""Check TLM-1 blocking put/get and analysis callbacks through native ports."""

import ctypes

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm


class Put(sc.channel_interface(tlm.tlm_blocking_put_if[int])):
    """Store samples after a blocking service delay."""

    def __init__(self, name):
        """Initialize channel identity and retained sample records."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.values = []

    def put(self, value):
        """Wait one nanosecond then retain the received value."""
        sc.wait(1, sc.SC_NS)
        self.values.append(int(value))


class Get:
    """Return value samples after waiting inside native virtual dispatch."""

    def __init__(self):
        """Initialize channel identity and response value."""
        self.value = 23

    def get(self):
        """Wait one nanosecond and return the next native integer value."""
        sc.wait(1, sc.SC_NS)
        return self.value


class Analysis(sc.channel_interface(tlm.tlm_analysis_if[int])):
    """Record analysis broadcasts without consuming simulation time."""

    def __init__(self, name):
        """Initialize channel identity and received values."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.values = []

    def write_value(self, value):
        """Retain a value broadcast through the native analysis port."""
        self.values.append(int(value))


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
put = Put("put")
get = Get()
analysis = Analysis("analysis")
put_port = sc.sc_port[tlm.tlm_blocking_put_if[int]]("put_port")
get_port = sc.sc_port[tlm.tlm_blocking_get_if[int]]("get_port")
analysis_port = tlm.tlm_analysis_port[int]("analysis_port")
put_port.bind(put)
get_port.bind(get)
analysis_port.bind(analysis)
del module_name
type(put_port.get_interface()).put.__release_gil__ = True
records = []


def process():
    """Assert blocking callback order and analysis broadcast values/time."""
    put_port.get_interface().put(17)
    value = get_port.get_interface().get()
    assert value == 23
    response = ctypes.c_int(-1)
    get_port.get_interface().get(response)
    assert response.value == 23
    analysis_port.write(value)
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(process, "process")
sc.sc_start(4, sc.SC_NS)
assert put.values == [17]
assert analysis.values == [23]
assert records == ["3 ns"], records
assert handle.terminated()
sc.sc_stop()
print("NATIVE_TLM1_BLOCKING_INTERFACES_OK")
