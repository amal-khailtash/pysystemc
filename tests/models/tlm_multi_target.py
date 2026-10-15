"""Verify multi target connection indices and independent borrowed transaction state."""

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

    def transport(self, index, payload, delay):
        """Suspend each connection and mutate its original transaction."""
        self.check(index, payload)
        sc.wait(delay)
        payload.set_address(21 + index)
        delay.__assign__(sc.SC_ZERO_TIME)
        records.append((index, str(sc.sc_time_stamp())))

    def forward(self, index, payload, phase, delay):
        """Preserve connection index and mutable native phase/delay."""
        self.check(index, payload)
        phase.__assign__(tlm.BEGIN_RESP)
        delay.__assign__(sc.sc_time(index + 1, sc.SC_NS))
        return tlm.TLM_UPDATED

    def debug(self, index, payload):
        """Return distinct counts to prove independent connection routing."""
        self.check(index, payload)
        return self.counts[index]

    def dmi(self, index, payload, result):
        """Reject DMI while supplying per-connection output ranges."""
        self.check(index, payload)
        result.set_start_address(17 + index)
        result.set_end_address(31 + index)
        result.allow_none()
        return False


target = Target()
target.counts = [7, 8]
retained = weakref.ref(target)
socket = tlm_utils.callback_multi_target_socket("target", width=73, connections=2)
socket.register_b_transport(target.transport)
socket.register_nb_transport_fw(target.forward)
socket.register_transport_dbg(target.debug)
socket.register_get_direct_mem_ptr(target.dmi)


def duplicate(*args):
    """Fail if duplicate registration replaces the first native callable."""
    raise AssertionError("duplicate callback replaced original")


socket.register_b_transport(duplicate)
socket.register_nb_transport_fw(duplicate)
socket.register_transport_dbg(duplicate)
socket.register_get_direct_mem_ptr(duplicate)
initiators = [tlm_utils.callback_initiator_socket(f"initiator_{i}", width=73) for i in range(2)]
for initiator in initiators:
    initiator.bind(socket)
del target, name
gc.collect()
assert retained() is not None
sc.sc_start(sc.SC_ZERO_TIME)
assert socket.size() == 2


def run(index):
    """Verify all forward callbacks then wait using automatic GIL release."""
    interface = initiators[index].get_interface()
    payload = payloads[index]
    delay = sc.sc_time(2 + index, sc.SC_NS)
    phase = tlm.tlm_phase(tlm.BEGIN_REQ)
    assert interface.nb_transport_fw(payload, phase, delay) == tlm.TLM_UPDATED
    assert phase == tlm.BEGIN_RESP and delay == sc.sc_time(index + 1, sc.SC_NS)
    assert interface.transport_dbg(payload) == index + 7
    for count in (0, 2**32 - 1):
        retained().counts[index] = count
        assert interface.transport_dbg(payload) == count
    for count in (-1, 2**32):
        retained().counts[index] = count
        try:
            interface.transport_dbg(payload)
        except Exception as error:
            assert "outside unsigned int range" in str(error), error
        else:
            raise AssertionError("invalid multi debug count accepted")
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
print("MULTI_TARGET_SOCKET_OK")
