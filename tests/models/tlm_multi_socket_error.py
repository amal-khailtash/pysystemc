"""Verify indexed multi transport errors preserve borrowed state and kernel time."""

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import cppyy

name = sc.sc_module_name("top")
top = sc.sc_module(name)
initiator = tlm_utils.callback_multi_initiator_socket("initiator", width=73)
target = tlm_utils.callback_multi_target_socket("target", width=73)
payload = tlm.tlm_generic_payload()
payload.set_address(17)
delay = sc.sc_time(2, sc.SC_NS)
records = []


def transport(index, trans, latency):
    """Mutate original transaction after waiting, then raise a visible Python error."""
    assert index == 0 and cppyy.addressof(trans) == cppyy.addressof(payload)
    try:
        sc.wait(latency)
        trans.set_address(21)
        latency.__assign__(sc.SC_ZERO_TIME)
        raise ValueError("multi socket callback failure")
    finally:
        records.append((index, str(sc.sc_time_stamp())))


target.register_b_transport(transport)
initiator.bind(target)
del name
sc.sc_start(sc.SC_ZERO_TIME)
interface = initiator[0]
assert interface.b_transport.__release_gil__
completed = []


def run():
    """Call indexed native transport and record only successful completion."""
    interface.b_transport(payload, delay)
    completed.append(True)


handle = sc.sc_spawn(run, "run")
try:
    sc.sc_start(sc.sc_time(3, sc.SC_NS))
except Exception as error:
    assert "multi socket callback failure" in str(error), error
else:
    raise AssertionError("multi transport exception swallowed")
assert completed == [] and records == [(0, "2 ns")], (completed, records)
assert payload.get_address() == 21 and delay == sc.SC_ZERO_TIME
assert sc.sc_time_stamp() == sc.sc_time(2, sc.SC_NS)
print("MULTI_SOCKET_ERROR_OK")
