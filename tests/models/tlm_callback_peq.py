"""Validate callback PEQ phase copies, payload identity, timing and owner retention."""

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import gc
import weakref

records = []


class Receiver:
    """Receive borrowed transactions without waiting in the method process."""

    def deliver(self, payload, phase):
        """Record native address/phase and mutate the original transaction."""
        records.append((int(payload.get_address()), str(phase), str(sc.sc_time_stamp()), int(sc.sc_delta_count())))
        payload.set_response_status(tlm.TLM_OK_RESPONSE)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
receiver = Receiver()
retained = weakref.ref(receiver)
queue = tlm_utils.callback_peq("queue", receiver.deliver)
first, second = tlm.tlm_generic_payload(), tlm.tlm_generic_payload()
first.set_address(17)
second.set_address(23)
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
queue.notify(first, phase, sc.SC_ZERO_TIME)
queue.notify(second, phase, sc.sc_time(2, sc.SC_NS))
phase.__assign__(tlm.END_RESP)
del receiver, name
gc.collect()
assert retained() is not None
sc.sc_start(3, sc.SC_NS)
assert [record[:3] for record in records] == [(17, "BEGIN_REQ", "0 s"), (23, "BEGIN_REQ", "2 ns")], records
assert records[0][3] > 0
assert first.is_response_ok() and second.is_response_ok()
queue.notify(first, phase, sc.sc_time(1, sc.SC_NS))
queue.cancel_all()
sc.sc_start(2, sc.SC_NS)
assert len(records) == 2
sc.sc_stop()
print("CALLBACK_PEQ_OK")
