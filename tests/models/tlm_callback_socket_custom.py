"""Verify custom protocol sockets preserve wide payload identity across transport callbacks."""

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import cppyy
import gc
import weakref


cppyy.cppdef("""
/** @brief Borrowed socket transaction with a native field wider than machine integers. */
struct SocketPayload : tlm::tlm_generic_payload { sc_dt::sc_biguint<137> value; };
/** @brief Preserve custom payload specialization through forward and backward sockets. */
struct SocketProtocol {
  using tlm_payload_type = SocketPayload;
  using tlm_phase_type = tlm::tlm_phase;
};
""")


def check_payload(trans):
    """Verify callback receives the original custom native object and its wide field."""
    assert cppyy.addressof(trans) == cppyy.addressof(payload)
    assert trans.value[136].to_bool() and trans.value.to_uint64() == 17
    callback_records.append("payload")


class Target:
    """Borrow transaction state and suspend through a registered blocking callback."""

    def nonblocking(self, payload, phase, delay):
        """Update native phase and latency without waiting."""
        check_payload(payload)
        phase.__assign__(tlm.BEGIN_RESP)
        delay.__assign__(sc.sc_time(1, sc.SC_NS))
        return tlm.TLM_UPDATED

    def debug(self, payload):
        """Return caller-controlled debug count for native range checks."""
        check_payload(payload)
        return self.count

    def dmi(self, payload, result):
        """Reject DMI with an explicit native range."""
        check_payload(payload)
        result.set_start_address(17)
        result.set_end_address(31)
        result.allow_none()
        return False

    def transport(self, payload, delay):
        """Consume two nanoseconds of annotation and mutate native response state."""
        check_payload(payload)
        sc.wait(delay)
        delay.__assign__(sc.SC_ZERO_TIME)
        payload.set_address(payload.get_address() + 4)
        payload.set_response_status(tlm.TLM_OK_RESPONSE)
        payload.value = cppyy.gbl.sc_dt.sc_biguint[137](23)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
target = Target()
retained = weakref.ref(target)
socket = tlm_utils.callback_target_socket("target", width=73, protocol=cppyy.gbl.SocketProtocol)
target.count = 0
socket.register_b_transport(target.transport)
socket.register_nb_transport_fw(target.nonblocking)
socket.register_transport_dbg(target.debug)
socket.register_get_direct_mem_ptr(target.dmi)
initiator = tlm_utils.callback_initiator_socket("initiator", width=73, protocol=cppyy.gbl.SocketProtocol)
backward_records = []


def response(payload, phase, delay):
    """Complete native backward response while preserving mutable phase."""
    check_payload(payload)
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
payload = cppyy.gbl.SocketPayload()
payload.value = cppyy.gbl.sc_dt.sc_biguint[137]("0x10000000000000000000000000000000011")
callback_records = []
payload.set_address(17)
delay = sc.sc_time(2, sc.SC_NS)
records = []
interface = initiator.get_interface()
assert interface.b_transport.__release_gil__
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
assert interface.nb_transport_fw(payload, phase, delay) == tlm.TLM_UPDATED
assert phase == tlm.BEGIN_RESP and delay == sc.sc_time(1, sc.SC_NS)
dmi = tlm.tlm_dmi()
assert not interface.get_direct_mem_ptr(payload, dmi)
assert (dmi.get_start_address(), dmi.get_end_address()) == (17, 31)
assert dmi.is_none_allowed()
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
back = socket.get_base_port().get_interface()
assert back.nb_transport_bw(payload, phase, delay) == tlm.TLM_COMPLETED
assert phase == tlm.END_RESP
back.invalidate_direct_mem_ptr(0, 2**64 - 1)
assert backward_records == ["response", (0, 2**64 - 1)]
delay.__assign__(sc.sc_time(2, sc.SC_NS))


def run():
    """Call the upstream socket path and observe preserved references/time."""
    interface.b_transport(payload, delay)
    assert payload.get_address() == 21 and payload.is_response_ok()
    assert delay == sc.SC_ZERO_TIME
    assert payload.value.to_uint64() == 23 and not payload.value[136].to_bool()
    records.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(run, "run")
sc.sc_start(3, sc.SC_NS)
assert handle.terminated() and records == ["2 ns"], records
assert callback_records == ["payload"] * 8, callback_records
print(
    f"CUSTOM_SOCKET {payload.get_address()} {payload.value.to_uint64()} {int(payload.value[136].to_bool())} {records[0]}"
)
sc.sc_stop()
print("CUSTOM_CALLBACK_SOCKET_OK")
