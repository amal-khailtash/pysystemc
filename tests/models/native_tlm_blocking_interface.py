"""Validate source TLM blocking transport references and Python process suspension."""

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm


class Target:
    """Implement native blocking transport with borrowed payload and delay references."""

    def __init__(self):
        """Create channel identity and retain observed request records."""
        self.requests = []

    def b_transport(self, trans, delay):
        """Consume annotated delay, wait for service, and mutate the original payload."""
        self.requests.append((int(trans.get_address()), str(delay)))
        sc.wait(delay)
        sc.wait(1, sc.SC_NS)
        delay.__assign__(sc.SC_ZERO_TIME)
        trans.set_address(trans.get_address() + 4)
        trans.set_response_status(tlm.TLM_OK_RESPONSE)


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
target = Target()
interface_type = tlm.tlm_blocking_transport_if[tlm.tlm_generic_payload]
port = sc.sc_port[interface_type]("port")
port.bind(target)
del module_name
trans = tlm.tlm_generic_payload()
trans.set_address(16)
trans.set_command(tlm.TLM_READ_COMMAND)
delay = sc.sc_time(2, sc.SC_NS)
records = []


def initiator():
    """Call through a native port and assert mutations survive virtual dispatch."""
    port.get_interface().b_transport(trans, delay)
    assert trans.get_address() == 20
    assert trans.is_response_ok()
    assert delay == sc.SC_ZERO_TIME
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(initiator, "initiator")
sc.sc_start(4, sc.SC_NS)
assert records == ["3 ns"], records
assert target.requests == [(16, "2 ns")], target.requests
assert handle.terminated()
sc.sc_stop()
print("NATIVE_TLM_BLOCKING_INTERFACE_OK")
