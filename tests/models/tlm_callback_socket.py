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

    def debug(self, payload):
        """Return caller-controlled debug count for native range checks."""
        return self.count

    def dmi(self, payload, result):
        """Reject DMI with an explicit native range."""
        result.set_start_address(17)
        result.set_end_address(31)
        result.allow_none()
        return False

    def transport(self, payload, delay):
        """Consume two nanoseconds of annotation and mutate native response state."""
        sc.wait(delay)
        delay.__assign__(sc.SC_ZERO_TIME)
        payload.set_address(payload.get_address() + 4)
        payload.set_response_status(tlm.TLM_OK_RESPONSE)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
target = Target()
retained = weakref.ref(target)
socket = tlm_utils.callback_target_socket("target", width=73)
target.count = 0
socket.register_b_transport(target.transport)
socket.register_nb_transport_fw(target.nonblocking)
socket.register_transport_dbg(target.debug)
socket.register_get_direct_mem_ptr(target.dmi)
initiator = tlm.tlm_initiator_socket[73]("initiator")
backward = sc.channel_interface(tlm.tlm_bw_transport_if[tlm.tlm_base_protocol_types])


class Backward(backward):
    """Provide the required backward native socket interface."""

    def nb_transport_bw(self, payload, phase, delay):
        """Complete unused backward traffic."""
        return tlm.TLM_COMPLETED

    def invalidate_direct_mem_ptr(self, start, end):
        """Accept unused invalidation."""


back_name = sc.sc_module_name("backward")
back = Backward(back_name)
initiator.bind(back)
initiator.bind(socket)
del back_name
del target, name
gc.collect()
assert retained() is not None
payload = tlm.tlm_generic_payload()
payload.set_address(17)
delay = sc.sc_time(2, sc.SC_NS)
records = []
interface = initiator.get_interface()
interface.b_transport.__release_gil__ = True
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
assert interface.nb_transport_fw(payload, phase, delay) == tlm.TLM_UPDATED
assert phase == tlm.BEGIN_RESP and delay == sc.sc_time(1, sc.SC_NS)
dmi = tlm.tlm_dmi()
assert not interface.get_direct_mem_ptr(payload, dmi)
assert (dmi.get_start_address(), dmi.get_end_address()) == (17, 31)
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
        raise AssertionError("invalid debug count accepted")
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
print("CALLBACK_SOCKET_OK")
