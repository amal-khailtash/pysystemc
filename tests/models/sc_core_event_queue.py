"""Validate complete event queue scheduling and borrowed interface contracts."""

from amal.eda.systemc import sc_core as sc
import cppyy

name = sc.sc_module_name("queues")
model = sc.sc_module(name)
queue = sc.sc_event_queue("queue")
interface = cppyy.bind_object(cppyy.addressof(queue), sc.sc_event_queue_if)
records = []


def observe():
    """Record each queued delivery, including equal-time separate deltas."""
    records.append((sc.sc_time_stamp().value(), sc.sc_delta_count()))


model.sc_method("observe", observe)
model.sensitive(queue.default_event())
model.dont_initialize()
assert queue.kind() == "sc_event_queue"
assert cppyy.addressof(queue.default_event()) == cppyy.addressof(queue.default_event())
assert sc.sc_event_queue.SC_CURRENT_USER_MODULE is sc.sc_event_queue
del name
interface.notify(sc.sc_time(1, sc.SC_NS))
interface.notify(1, sc.SC_NS)
queue.notify(2, sc.SC_NS)
sc.sc_start(3, sc.SC_NS)
assert [time for time, delta in records] == [1000, 1000, 2000]
assert records[0][1] < records[1][1]
interface.notify(1, sc.SC_NS)
interface.cancel_all()
queue.notify(sc.sc_time(2, sc.SC_NS))
queue.cancel_all()
queue.cancel_all()
sc.sc_start(3, sc.SC_NS)
assert len(records) == 3
queue.notify(sc.sc_time(1, sc.SC_NS))
sc.sc_start(2, sc.SC_NS)
assert [time for time, delta in records] == [1000, 1000, 2000, 7000]
sc.sc_stop()
for time, delta in records:
    print("EVENT_QUEUE_REFERENCE", time, delta)
