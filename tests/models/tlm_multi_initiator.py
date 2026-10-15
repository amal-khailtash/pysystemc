"""Verify multi initiator indexed forward and connection-indexed backward callbacks."""

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import cppyy
import gc
import weakref

name = sc.sc_module_name("top")
top = sc.sc_module(name)
records = []
payloads = [tlm.tlm_generic_payload(), tlm.tlm_generic_payload()]


class Target:
    """Retain callbacks while upstream multi socket supplies connection indices."""

    def check(self, index, payload):
        """Verify native identity matches the calling connection."""
        assert cppyy.addressof(payload) == cppyy.addressof(payloads[index])

    def transport(self, payload, delay):
        """Suspend each connection and mutate its original transaction."""
        index = payloads.index(payload)
        self.check(index, payload)
        sc.wait(delay)
        payload.set_address(21 + index)
        delay.__assign__(sc.SC_ZERO_TIME)
        records.append((index, str(sc.sc_time_stamp())))

    def forward(self, payload, phase, delay):
        """Preserve connection index and mutable native phase/delay."""
        index = payloads.index(payload)
        self.check(index, payload)
        phase.__assign__(tlm.BEGIN_RESP)
        delay.__assign__(sc.sc_time(index + 1, sc.SC_NS))
        return tlm.TLM_UPDATED

    def debug(self, payload):
        """Return distinct counts to prove independent connection routing."""
        index = payloads.index(payload)
        self.check(index, payload)
        return index + 7

    def dmi(self, payload, result):
        """Reject DMI while supplying per-connection output ranges."""
        index = payloads.index(payload)
        self.check(index, payload)
        result.set_start_address(17 + index)
        result.set_end_address(31 + index)
        result.allow_none()
        return False


target = Target()
retained = weakref.ref(target)
sockets = [tlm_utils.callback_target_socket(f"target_{i}", width=73) for i in range(2)]
for socket in sockets:
    socket.register_b_transport(target.transport)
    socket.register_nb_transport_fw(target.forward)
    socket.register_transport_dbg(target.debug)
    socket.register_get_direct_mem_ptr(target.dmi)
initiator = tlm_utils.callback_multi_initiator_socket("initiator", width=73, connections=2)
backward_records = []


def response(index, payload, phase, delay):
    """Verify backward connection identity and mutable phase/delay."""
    assert cppyy.addressof(payload) == cppyy.addressof(payloads[index])
    phase.__assign__(tlm.END_RESP)
    delay.__assign__(sc.sc_time(index + 1, sc.SC_NS))
    backward_records.append(("response", index))
    return tlm.TLM_COMPLETED


def invalidate(index, first, last):
    """Verify full-width invalidation retains connection ID."""
    assert (int(first), int(last)) == (0, 2**64 - 1)
    backward_records.append(("invalidate", index))


initiator.register_nb_transport_bw(response)
initiator.register_invalidate_direct_mem_ptr(invalidate)


def duplicate(*args):
    """Fail if duplicate registration replaces the first native callable."""
    raise AssertionError("duplicate callback replaced original")


initiator.register_nb_transport_bw(duplicate)
initiator.register_invalidate_direct_mem_ptr(duplicate)
for socket in sockets:
    initiator.bind(socket)
del target, name
gc.collect()
assert retained() is not None
sc.sc_start(sc.SC_ZERO_TIME)
assert initiator.size() == 2
for index, socket in enumerate(sockets):
    back = socket.get_base_port().get_interface()
    phase = tlm.tlm_phase(tlm.BEGIN_RESP)
    delay = sc.sc_time(0, sc.SC_NS)
    assert back.nb_transport_bw(payloads[index], phase, delay) == tlm.TLM_COMPLETED
    assert phase == tlm.END_RESP and delay == sc.sc_time(index + 1, sc.SC_NS)
    back.invalidate_direct_mem_ptr(0, 2**64 - 1)
assert backward_records == [("response", 0), ("invalidate", 0), ("response", 1), ("invalidate", 1)]


def run(index):
    """Verify all forward callbacks then wait using automatic GIL release."""
    interface = initiator[index]
    payload = payloads[index]
    delay = sc.sc_time(2 + index, sc.SC_NS)
    phase = tlm.tlm_phase(tlm.BEGIN_REQ)
    assert interface.nb_transport_fw(payload, phase, delay) == tlm.TLM_UPDATED
    assert phase == tlm.BEGIN_RESP and delay == sc.sc_time(index + 1, sc.SC_NS)
    assert interface.transport_dbg(payload) == index + 7
    result = tlm.tlm_dmi()
    assert not interface.get_direct_mem_ptr(payload, result)
    assert (result.get_start_address(), result.get_end_address()) == (17 + index, 31 + index)
    assert result.is_none_allowed()
    delay.__assign__(sc.sc_time(2 + index, sc.SC_NS))
    assert interface.b_transport.__release_gil__
    interface.b_transport(payload, delay)
    assert payload.get_address() == 21 + index and delay == sc.SC_ZERO_TIME


handles = [sc.sc_spawn(lambda i=i: run(i), f"run_{i}") for i in range(2)]
sc.sc_start(4, sc.SC_NS)
assert all(handle.terminated() for handle in handles)
assert records == [(0, "2 ns"), (1, "3 ns")], records
sc.sc_stop()
print("MULTI_INITIATOR_SOCKET_OK")
