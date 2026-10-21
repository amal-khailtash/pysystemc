"""Compare deprecated free notify overloads, native initialization and pause/resume."""

from amal.eda.systemc import sc_core as sc

records = []
thread_records = []
name = sc.sc_module_name("control")
model = sc.sc_module(name)
event = sc.sc_event("trigger")


def observer():
    """Record actual event callback time/delta and pause on the timed callback."""
    records.append((sc.sc_time_stamp().value(), sc.sc_delta_count()))
    if sc.sc_time_stamp() == sc.sc_time(2, sc.SC_NS):
        sc.sc_pause()


model.sc_method("observer", observer)
model.sensitive(event)
model.dont_initialize()


def initializer_thread():
    """Run during initialization and resume after a native timed wait."""
    thread_records.append(sc.sc_time_stamp().value())
    sc.wait(1, sc.SC_NS)
    thread_records.append(sc.sc_time_stamp().value())


model.sc_thread("initializer", initializer_thread)
del name
sc.sc_initialize()
assert thread_records == [0]
assert records == [] and sc.sc_time_stamp() == sc.SC_ZERO_TIME
sc.sc_start(sc.SC_ZERO_TIME)
sc.notify(event)
sc.sc_start(sc.SC_ZERO_TIME)
assert len(records) == 1
sc.notify(sc.SC_ZERO_TIME, event)
sc.sc_start(sc.SC_ZERO_TIME)
assert len(records) == 1
sc.sc_start(sc.SC_ZERO_TIME)
assert len(records) == 2
sc.notify(2, sc.SC_NS, event)
sc.sc_start(10, sc.SC_NS)
assert sc.sc_time_stamp() == sc.sc_time(2, sc.SC_NS) and len(records) == 3
assert sc.sc_get_status() == sc.SC_PAUSED
sc.notify(sc.sc_time(1, sc.SC_NS), event)
sc.sc_start(2, sc.SC_NS, sc.SC_RUN_TO_TIME)
assert sc.sc_time_stamp() == sc.sc_time(4, sc.SC_NS) and len(records) == 4
assert [time for time, delta in records] == [0, 0, 2000, 3000]
assert thread_records == [0, 1000]
sc.sc_stop()
for index, (time, delta) in enumerate(records):
    print("NOTIFY_CONTROL_REFERENCE", index, time, delta)
print("NOTIFY_FINAL_REFERENCE", sc.sc_time_stamp().value(), int(sc.sc_get_status()))
