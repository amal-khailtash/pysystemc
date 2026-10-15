"""Exercise combined native TLM forward/backward Python interface implementations."""

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm

protocol = tlm.tlm_base_protocol_types


class Target(sc.channel_interface(tlm.tlm_fw_transport_if[protocol])):
    """Implement all forward methods without waiting in nonblocking or debug calls."""

    def __init__(self, name):
        """Initialize native channel identity and callback records."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.calls = []
        self.debug_result = 0

    def nb_transport_fw(self, trans, phase, delay):
        """Return an updated response phase and annotate one nanosecond latency."""
        self.calls.append("fw")
        phase.__assign__(tlm.BEGIN_RESP)
        delay.__assign__(sc.sc_time(1, sc.SC_NS))
        trans.set_response_status(tlm.TLM_OK_RESPONSE)
        return tlm.TLM_UPDATED

    def b_transport(self, trans, delay):
        """Complete blocking transport immediately for this interface matrix."""
        self.calls.append("blocking")
        trans.set_response_status(tlm.TLM_OK_RESPONSE)

    def debug_transport(self, trans):
        """Return zero bytes without consuming simulation time or notifying events."""
        self.calls.append("debug")
        return self.debug_result

    def get_direct_mem_ptr(self, trans, dmi):
        """Reject DMI and report the address range covered by the rejection."""
        self.calls.append("dmi")
        dmi.set_start_address(16)
        dmi.set_end_address(31)
        dmi.allow_none()
        return False


class Initiator(sc.channel_interface(tlm.tlm_bw_transport_if[protocol])):
    """Implement response completion and DMI invalidation callbacks."""

    def __init__(self, name):
        """Initialize channel identity and retained invalidation records."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.invalidations = []

    def nb_transport_bw(self, trans, phase, delay):
        """Complete the response while preserving borrowed transaction identity."""
        assert trans.is_response_ok()
        phase.__assign__(tlm.END_RESP)
        return tlm.TLM_COMPLETED

    def invalidate_direct_mem_ptr(self, start, end):
        """Record the invalidated range without requesting DMI recursively."""
        self.invalidations.append((int(start), int(end)))


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
target = Target("target")
initiator = Initiator("initiator")
fw_port = sc.sc_port[tlm.tlm_fw_transport_if[protocol]]("fw")
bw_port = sc.sc_port[tlm.tlm_bw_transport_if[protocol]]("bw")
fw_port.bind(target)
bw_port.bind(initiator)
del module_name
trans = tlm.tlm_generic_payload()
trans.set_address(16)
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
delay = sc.sc_time(0, sc.SC_NS)
dmi = tlm.tlm_dmi()
fw = fw_port.get_interface()
bw = bw_port.get_interface()
assert fw.nb_transport_fw(trans, phase, delay) == tlm.TLM_UPDATED
assert phase == tlm.BEGIN_RESP
assert delay == sc.sc_time(1, sc.SC_NS)
assert bw.nb_transport_bw(trans, phase, delay) == tlm.TLM_COMPLETED
assert phase == tlm.END_RESP
assert fw.transport_dbg(trans) == 0
assert not fw.get_direct_mem_ptr(trans, dmi)
assert (dmi.get_start_address(), dmi.get_end_address()) == (16, 31)
assert not dmi.is_read_allowed() and not dmi.is_write_allowed()
bw.invalidate_direct_mem_ptr(16, 31)
assert initiator.invalidations == [(16, 31)]
fw.b_transport(trans, delay)
assert target.calls == ["fw", "debug", "dmi", "blocking"]
target.debug_result = 2**32 - 1
assert fw.transport_dbg(trans) == 2**32 - 1
for invalid in (-1, 2**32):
    target.debug_result = invalid
    try:
        fw.transport_dbg(trans)
    except Exception as error:
        assert "outside unsigned int range" in str(error), error
    else:
        raise AssertionError(f"accepted invalid debug count {invalid}")
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
print("NATIVE_TLM_COMBINED_INTERFACES_OK")
