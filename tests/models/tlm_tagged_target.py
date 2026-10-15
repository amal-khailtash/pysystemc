"""Verify native convenience socket member-pointer forwarding to Python callbacks."""

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import gc
import weakref


class Target:
    """Borrow transaction state and suspend through a registered blocking callback."""

    def nonblocking(self, payload, phase, delay):
        """Update native phase and latency without waiting."""
        phase.__assign__(tlm.BEGIN_RESP)
        delay.__assign__(sc.sc_time(1, sc.SC_NS))
        return tlm.TLM_UPDATED

    def debug(self, identifier, payload):
        """Return caller-controlled debug count for native range checks."""
        assert identifier == 59
        return self.count

    def dmi(self, identifier, payload, result):
        """Reject DMI with an explicit native range."""
        assert identifier == 61
        result.set_start_address(17)
        result.set_end_address(31)
        result.allow_none()
        return False

    def transport(self, identifier, payload, delay):
        """Consume two nanoseconds of annotation and mutate native response state."""
        assert identifier == 47
        sc.wait(delay)
        delay.__assign__(sc.SC_ZERO_TIME)
        payload.set_address(payload.get_address() + 4)
        payload.set_response_status(tlm.TLM_OK_RESPONSE)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
target = Target()
retained = weakref.ref(target)
socket = tlm_utils.callback_target_socket("target", width=73, tagged=True)
target.count = 0
socket.register_b_transport(target.transport, 47)
socket.register_transport_dbg(target.debug, 59)
socket.register_get_direct_mem_ptr(target.dmi, 61)
initiator = tlm_utils.callback_initiator_socket("initiator", width=73)
backward_records = []


def response(payload, phase, delay):
    """Complete native backward response while preserving mutable phase."""
    phase.__assign__(tlm.END_RESP)
    backward_records.append("response")
    return tlm.TLM_COMPLETED


def invalidate(first, last):
    """Record full-width native invalidation ranges."""
    backward_records.append((int(first), int(last)))


initiator.register_nb_transport_bw(response)
initiator.register_invalidate_direct_mem_ptr(invalidate)
initiator.bind(socket)
del target, name
gc.collect()
assert retained() is not None
payload = tlm.tlm_generic_payload()
payload.set_address(17)
delay = sc.sc_time(2, sc.SC_NS)
records = []
interface = initiator.get_interface()
assert interface.b_transport.__release_gil__
dmi = tlm.tlm_dmi()
assert not interface.get_direct_mem_ptr(payload, dmi)
assert (dmi.get_start_address(), dmi.get_end_address()) == (17, 31)
assert not dmi.is_read_allowed() and not dmi.is_write_allowed()
for count in (0, 2**32 - 1):
    retained().count = count
    assert interface.transport_dbg(payload) == count
for count in (-1, 2**32):
    retained().count = count
    try:
        interface.transport_dbg(payload)
    except Exception as error:
        assert "outside unsigned int range" in str(error), error
    else:
        raise AssertionError("invalid tagged debug count accepted")
delay.__assign__(sc.sc_time(2, sc.SC_NS))


def run():
    """Call the upstream socket path and observe preserved references/time."""
    interface.b_transport(payload, delay)
    assert payload.get_address() == 21 and payload.is_response_ok()
    assert delay == sc.SC_ZERO_TIME
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(run, "run")
sc.sc_start(3, sc.SC_NS)
assert handle.terminated() and records == ["2 ns"], records
sc.sc_stop()
print("TAGGED_TARGET_SOCKET_OK")
