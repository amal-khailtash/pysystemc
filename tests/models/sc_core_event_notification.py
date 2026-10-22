"""Compare event member notification priority, cancellation and triggered state."""

from amal.eda.systemc import sc_core as sc

records = []
name = sc.sc_module_name("events")
model = sc.sc_module(name)
event = sc.sc_event("event")


def observer():
    """Record native timestamp and event trigger state during callback dispatch."""
    assert event.triggered()
    records.append(sc.sc_time_stamp().value())


def drive():
    """Exercise immediate/time/unit notification, earlier priority and cancellation."""
    sc.wait(1, sc.SC_NS)
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
    event.notify(sc.SC_ZERO_TIME)
    sc.wait(1, sc.SC_NS)
    assert not event.triggered()
    sc.sc_stop()


model.sc_method("observer", observer)
model.sensitive(event)
model.dont_initialize()
model.sc_thread("driver", drive)
del name
sc.sc_start()
assert records == [1000, 3000, 5000, 8000, 11000], records
print("EVENT_NOTIFY_REFERENCE", *records)
