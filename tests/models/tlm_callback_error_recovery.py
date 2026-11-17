"""Exercise public TLM adapter errors followed by native callback recovery."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils

import cppyy


def reject(call, error_type, detail):
    """Require a public negative call to raise its documented exception."""
    try:
        call()
    except error_type as error:
        assert detail in str(error), error
    else:
        raise AssertionError(f"invalid public call was accepted: {detail}")


module_name = sc.sc_module_name("top")
top = sc.sc_module(module_name)
partial_fw = tlm.tlm_fw_nonblocking_transport_if[tlm.tlm_generic_payload, tlm.tlm_phase]
forward_port = sc.sc_port[partial_fw]("forward_port")


class InvalidProtocol:
    """Expose a malformed protocol declaration to structural binding."""

    protocol_types = object()

    def nb_transport_fw(self, payload, phase, delay):
        """Return a native result if the invalid instance is ever dispatched."""
        return tlm.TLM_COMPLETED


reject(lambda: forward_port.bind(InvalidProtocol()), TypeError, "protocol_types must name a native TLM protocol class")


class ForwardImplementation:
    """Implement a valid base-protocol forward transport endpoint."""

    protocol_types = tlm.tlm_base_protocol_types

    def __init__(self):
        """Initialize retained native-call observations."""
        self.calls = []
        self.result = tlm.tlm_sync_enum(7)

    def nb_transport_fw(self, payload, phase, delay):
        """Return the configured result and record its borrowed request."""
        self.calls.append((int(payload.get_address()), str(delay), str(sc.sc_time_stamp())))
        return self.result

    def b_transport(self, payload, delay):
        """Satisfy the required blocking method without consuming time."""

    def transport_dbg(self, payload):
        """Return no transferred bytes for debug transport."""
        return 0

    def get_direct_mem_ptr(self, payload, dmi):
        """Decline direct memory access."""
        return False


forward = ForwardImplementation()
forward_port.bind(forward)

# Include the existing native probe before using its public member-pointer type.
cppyy.include(str(Path(__file__).resolve().parents[1] / "native/tlm_utils_multi_probe.h"))
probe = cppyy.gbl.pysystemc_tlm_utils_multi
multi_initiator_type = tlm_utils.multi_passthrough_initiator_socket[
    probe.Owner, 73, probe.Protocol, 0, sc.SC_ZERO_OR_MORE_BOUND
]

reject(
    lambda: tlm_utils.callback_peq("invalid_queue", None),
    TypeError,
    "payload queue callback must be callable",
)
reject(
    lambda: multi_initiator_type.nb_cb.from_member("Owner"),
    TypeError,
    "member must be a qualified C++ member name",
)
reject(
    lambda: getattr(tlm_utils, "not_a_native_tlm_utility"),
    AttributeError,
    "not_a_native_tlm_utility",
)

socket_target = tlm_utils.callback_target_socket("socket_target")
socket_initiator = tlm_utils.callback_initiator_socket("socket_initiator")
socket_results = [tlm.tlm_sync_enum(7)]
socket_calls = []


def socket_callback(payload, phase, delay):
    """Return the configured native sync value and observe public dispatch."""
    socket_calls.append((int(payload.get_address()), str(delay), str(sc.sc_time_stamp())))
    return socket_results[0]


socket_target.register_nb_transport_fw(socket_callback)
socket_initiator.bind(socket_target)
socket_payload = tlm.tlm_generic_payload()
socket_payload.set_address(73)
socket_phase = tlm.tlm_phase(tlm.BEGIN_REQ)
socket_delay = sc.sc_time(0, sc.SC_NS)
socket_forward = socket_initiator.get_interface()
reject(
    lambda: socket_forward.nb_transport_fw(socket_payload, socket_phase, socket_delay),
    ValueError,
    "invalid tlm_sync_enum",
)
socket_results[0] = tlm.TLM_COMPLETED
assert socket_forward.nb_transport_fw(socket_payload, socket_phase, socket_delay) == tlm.TLM_COMPLETED
assert socket_calls == [(73, "0 s", "0 s"), (73, "0 s", "0 s")], socket_calls

multi_owner = probe.Owner(sc.sc_module_name("multi_owner"))
multi_callback = multi_initiator_type.nb_cb.from_member("pysystemc_tlm_utils_multi::Owner::initiator_nb")
assert multi_callback.valid()
assert multi_initiator_type.register_nb_transport_bw(multi_owner.outer_initiator, multi_owner, multi_callback) is None
multi_owner.register_target_callbacks()
multi_owner.bind_paths()

peq_records = []


def deliver(payload, phase):
    """Record borrowed PEQ payload data, phase and native dispatch time."""
    peq_records.append((int(payload.get_address()), int(phase), str(sc.sc_time_stamp())))


queue = tlm_utils.callback_peq("recovery_queue", deliver)
peq_payload = tlm.tlm_generic_payload()
peq_payload.set_address(601)
peq_phase = tlm.tlm_phase(tlm.BEGIN_REQ)
queue.notify(peq_payload, peq_phase, sc.sc_time(2, sc.SC_NS))
del module_name

sc.sc_start(sc.SC_ZERO_TIME)
forward_interface = forward_port.get_interface()
reject(
    lambda: forward_interface.nb_transport_fw(socket_payload, socket_phase, socket_delay),
    ValueError,
    "invalid tlm_sync_enum",
)
forward.result = tlm.TLM_COMPLETED
assert forward_interface.nb_transport_fw(socket_payload, socket_phase, socket_delay) == tlm.TLM_COMPLETED
assert forward.calls == [(73, "0 s", "0 s"), (73, "0 s", "0 s")], forward.calls

multi_payload = probe.Payload()
multi_payload.set_address(0xA000)
multi_phase = tlm.tlm_phase(tlm.BEGIN_REQ)
multi_delay = sc.sc_time(0, sc.SC_NS)
assert multi_owner.outer_initiator[0].nb_transport_fw(multi_payload, multi_phase, multi_delay) == tlm.TLM_UPDATED
assert multi_phase == tlm.BEGIN_RESP
assert multi_delay == sc.sc_time(20, sc.SC_NS)
assert multi_owner.initiator_nb_calls[0] == 1
assert multi_owner.last_backward_phase[0] == int(tlm.END_RESP)
assert multi_owner.last_backward_delay[0] == int(sc.sc_time(60, sc.SC_NS).value())

sc.sc_start(sc.sc_time(3, sc.SC_NS))
assert sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
assert peq_records == [(601, int(tlm.BEGIN_REQ), "2 ns")]
print("TLM_CALLBACK_RECOVERY_SOCKET", *socket_calls[-1])
print("TLM_CALLBACK_RECOVERY_PEQ", *peq_records[0])
print("TLM_CALLBACK_RECOVERY_PROTOCOL", *forward.calls[-1])
print("TLM_CALLBACK_ERROR_RECOVERY_OK")
