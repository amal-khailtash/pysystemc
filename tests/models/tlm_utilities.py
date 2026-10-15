"""Validate native utility namespace, quantum timing and borrowed PEQ delivery."""

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import cppyy

name = sc.sc_module_name("top")
top = sc.sc_module(name)
queue = tlm_utils.peq_with_get[tlm.tlm_generic_payload]("queue")
keeper = tlm_utils.tlm_quantumkeeper()
keeper.set_global_quantum(sc.sc_time(5, sc.SC_NS))
keeper.reset()
a, b = tlm.tlm_generic_payload(), tlm.tlm_generic_payload()
a.set_address(17)
b.set_address(23)
queue.notify(a, sc.sc_time(2, sc.SC_NS))
queue.notify(b, sc.sc_time(4, sc.SC_NS))
del name
records = []


def quantum_process():
    """Synchronize local time at global quantum boundaries with released GIL."""
    keeper.inc(sc.sc_time(3, sc.SC_NS))
    assert not keeper.need_sync()
    assert keeper.get_current_time() == sc.sc_time(3, sc.SC_NS)
    keeper.inc(sc.sc_time(2, sc.SC_NS))
    assert keeper.need_sync()
    keeper.sync()
    assert sc.sc_time_stamp() == sc.sc_time(5, sc.SC_NS)
    assert keeper.get_local_time() == sc.SC_ZERO_TIME
    keeper.set_and_sync(sc.sc_time(5, sc.SC_NS))
    assert sc.sc_time_stamp() == sc.sc_time(10, sc.SC_NS)
    records.append(("quantum", "10 ns"))


def receiver():
    """Drain transactions only when due and preserve native object identity."""
    for expected, time in ((a, "2 ns"), (b, "4 ns")):
        sc.wait(queue.get_event())
        payload = queue.get_next_transaction()
        assert cppyy.addressof(payload) == cppyy.addressof(expected)
        assert str(sc.sc_time_stamp()) == time
        records.append((int(payload.get_address()), time))
        assert not queue.get_next_transaction()
    queue.notify(a, sc.sc_time(1, sc.SC_NS))
    queue.cancel_all()
    sc.wait(2, sc.SC_NS)
    assert not queue.get_next_transaction()


handles = [sc.sc_spawn(quantum_process, "quantum"), sc.sc_spawn(receiver, "receiver")]
sc.sc_start(11, sc.SC_NS)
assert records == [(17, "2 ns"), (23, "4 ns"), ("quantum", "10 ns")], records
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("TLM_UTILITIES_OK")
