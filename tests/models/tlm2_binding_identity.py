"""Check standard TLM2 protocol identity across partial-first binding orders."""

from amal.eda.systemc import sc_core as sc, tlm
import cppyy


class Owner:
    """Implement forward/backward callbacks over shared transaction state."""

    def __init__(self):
        """Initialize the ordered callback log."""
        self.calls = []

    def nb_transport_fw(self, payload, phase, delay):
        """Mutate borrowed payload, phase and delay without waiting."""
        self.calls.append("fw")
        payload.set_address(73)
        phase.__assign__(tlm.BEGIN_RESP)
        delay.__assign__(sc.sc_time(1, sc.SC_NS))
        return tlm.TLM_UPDATED

    def b_transport(self, payload, delay):
        """Set the borrowed response status without waiting."""
        self.calls.append("blocking")
        payload.set_response_status(tlm.TLM_OK_RESPONSE)

    def transport_dbg(self, payload):
        """Report the current shared payload address."""
        self.calls.append("debug")
        return int(payload.get_address())

    def get_direct_mem_ptr(self, payload, dmi):
        """Reject DMI while returning a borrowed address range."""
        self.calls.append("dmi")
        dmi.set_start_address(payload.get_address())
        dmi.set_end_address(79)
        return False

    def nb_transport_bw(self, payload, phase, delay):
        """Complete the response without retaining borrowed arguments."""
        self.calls.append("bw")
        phase.__assign__(tlm.END_RESP)
        return tlm.TLM_COMPLETED

    def invalidate_direct_mem_ptr(self, start, end):
        """Record invalidation of the shared DMI range."""
        self.calls.append((int(start), int(end)))


cppyy.cppdef("""
/** Borrow a native virtual interface address; no callbacks, GIL or retained storage. */
std::uintptr_t transport_identity(sc_core::sc_interface* value) {
  return reinterpret_cast<std::uintptr_t>(value);
}
""")
name = sc.sc_module_name("top")
top = sc.sc_module(name)
fw_types = (
    tlm.tlm_fw_transport_if[tlm.tlm_base_protocol_types],
    tlm.tlm_fw_nonblocking_transport_if[tlm.tlm_generic_payload, tlm.tlm_phase],
    tlm.tlm_blocking_transport_if[tlm.tlm_generic_payload],
    tlm.tlm_fw_direct_mem_if[tlm.tlm_generic_payload],
    tlm.tlm_transport_dbg_if[tlm.tlm_generic_payload],
)
bw_types = (
    tlm.tlm_bw_transport_if[tlm.tlm_base_protocol_types],
    tlm.tlm_bw_nonblocking_transport_if[tlm.tlm_generic_payload, tlm.tlm_phase],
    tlm.tlm_bw_direct_mem_if,
)
groups = []
for types in (fw_types, bw_types):
    for first in range(len(types)):
        owner = Owner()
        ports = {}
        for index in (first, *(index for index in range(len(types)) if index != first)):
            port = sc.sc_port[types[index]]()
            port(owner)
            ports[index] = port
        groups.append((types, owner, ports))
del name
for types, owner, ports in groups:
    addresses = [cppyy.gbl.transport_identity(port.get_interface()) for port in ports.values()]
    assert len(set(addresses)) == 1, addresses
    payload = tlm.tlm_generic_payload()
    phase = tlm.tlm_phase(tlm.BEGIN_REQ)
    delay = sc.sc_time(0, sc.SC_NS)
    if types is fw_types:
        assert ports[1].get_interface().nb_transport_fw(payload, phase, delay) == tlm.TLM_UPDATED
        assert payload.get_address() == 73 and phase == tlm.BEGIN_RESP
        assert delay == sc.sc_time(1, sc.SC_NS)
        ports[2].get_interface().b_transport(payload, delay)
        assert payload.is_response_ok()
        assert ports[4].get_interface().transport_dbg(payload) == 73
        dmi = tlm.tlm_dmi()
        assert not ports[3].get_interface().get_direct_mem_ptr(payload, dmi)
        assert (dmi.get_start_address(), dmi.get_end_address()) == (73, 79)
        assert owner.calls == ["fw", "blocking", "debug", "dmi"]
    else:
        assert ports[1].get_interface().nb_transport_bw(payload, phase, delay) == tlm.TLM_COMPLETED
        assert phase == tlm.END_RESP
        ports[2].get_interface().invalidate_direct_mem_ptr(73, 79)
        assert owner.calls == ["bw", (73, 79)]
sc.sc_start(2, sc.SC_NS)
sc.sc_stop()
print("TLM2_BINDING_IDENTITY 5 3 2 ns")
