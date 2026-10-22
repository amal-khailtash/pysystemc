"""Compare event member notification priority, cancellation and triggered state."""

from amal.eda.systemc import sc_core as sc
import cppyy

records = []
none_address = cppyy.addressof(sc.sc_event.none)
assert not sc.sc_event.none.triggered()
assert sc.sc_event.none.in_hierarchy()
assert sc.sc_event.none.basename() == "$$$$kernel_event$$$$_none"
assert not sc.sc_event.none.get_parent_object()

errors = []
name = sc.sc_module_name("events")
model = sc.sc_module(name)
event = sc.sc_event("event")
try:
    event.notify()
except sc.sc_report as report:
    assert report.get_msg_type() == sc.SC_ID_IMMEDIATE_NOTIFICATION_
else:
    raise AssertionError("immediate elaboration notification did not report")


def observer():
    """Record native timestamp and event trigger state during callback dispatch."""
    assert event.triggered()
    records.append(sc.sc_time_stamp().value())


def drive():
    """Exercise immediate/time/unit notification, earlier priority and cancellation."""
    sc.wait(1, sc.SC_NS)
    event.notify(10, sc.SC_NS)
    event.notify()
    sc.wait(1, sc.SC_NS)
    event.notify(sc.sc_time(1, sc.SC_NS))
    sc.wait(2, sc.SC_NS)
    event.notify(1, sc.SC_NS)
    sc.wait(2, sc.SC_NS)
    event.notify(5, sc.SC_NS)
    event.notify(2, sc.SC_NS)
    event.notify(3, sc.SC_NS)
    sc.wait(3, sc.SC_NS)
    event.notify(1, sc.SC_NS)
    event.cancel()
    event.cancel()
    sc.wait(2, sc.SC_NS)
    assert not event.triggered()
    event.notify(5, sc.SC_NS)
    event.notify(sc.SC_ZERO_TIME)
    event.notify(sc.SC_ZERO_TIME)
    event.notify(sc.sc_time(2, sc.SC_NS))
    event.notify(3, sc.SC_NS)
    sc.wait(1, sc.SC_NS)
    assert not event.triggered()
    for args in ((), (sc.sc_time(1, sc.SC_NS),), (1, sc.SC_NS)):
        event.notify_delayed(*args)
        try:
            event.notify_delayed(*args)
        except sc.sc_report as report:
            assert report.get_msg_type() == sc.SC_ID_NOTIFY_DELAYED_
            errors.append(1)
        else:
            raise AssertionError("duplicate delayed notification did not report")
        sc.wait(2, sc.SC_NS)
    sc.sc_stop()


model.sc_method("observer", observer)
model.sensitive(event)
model.dont_initialize()
model.sc_thread("driver", drive)
del name
for label, delay in (("discard_delta", sc.SC_ZERO_TIME), ("discard_timed", sc.sc_time(1, sc.SC_NS))):
    discarded = sc.sc_event(label)
    assert sc.sc_find_event(label)
    discarded.notify(delay)
    discarded.__destruct__()
    assert not sc.sc_find_event(label)
sc.sc_start()
assert not sc.sc_event.none.triggered()
assert cppyy.addressof(sc.sc_event.none) == none_address
assert records == [1000, 3000, 5000, 8000, 11000, 12000, 15000, 17000], records
assert errors == [1, 1, 1]
print("EVENT_NOTIFY_REFERENCE", *records)
print("EVENT_DELAYED_REFERENCE", len(errors))
