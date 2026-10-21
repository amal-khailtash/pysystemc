"""Verify native object/event lookup, parent hierarchy and null-result contracts."""

from amal.eda.systemc import sc_core as sc
import cppyy

name = sc.sc_module_name("lookup")
top = sc.sc_module(name)
signal = sc.sc_signal[int]("signal")
event = sc.sc_event("event")
assert cppyy.addressof(sc.sc_get_current_object()) == cppyy.addressof(top)
del name
assert not sc.sc_get_current_object()
assert cppyy.addressof(sc.sc_find_object("lookup.signal")) == cppyy.addressof(signal)
assert cppyy.addressof(sc.sc_find_event("lookup.event")) == cppyy.addressof(event)
assert not sc.sc_find_object("missing") and not sc.sc_find_event("missing")
assert cppyy.addressof(sc.sc_get_parent(signal)) == cppyy.addressof(top)
assert not sc.sc_get_parent(top)
context = sc.sc_get_curr_simcontext()
assert list(sc.sc_get_top_level_events()) == []
root_event = sc.sc_event("root_event")
assert context and cppyy.addressof(context) == cppyy.addressof(sc.sc_get_curr_simcontext())
for explicit in (False, True):
    objects = sc.sc_get_top_level_objects(context) if explicit else sc.sc_get_top_level_objects()
    events = sc.sc_get_top_level_events(context) if explicit else sc.sc_get_top_level_events()
    assert any(cppyy.addressof(obj) == cppyy.addressof(top) for obj in objects)
    assert all(cppyy.addressof(value) != cppyy.addressof(event) for value in events)
    assert any(cppyy.addressof(value) == cppyy.addressof(root_event) for value in events)
for lookup in (sc.sc_get_top_level_objects, sc.sc_get_top_level_events):
    try:
        lookup(object())
    except TypeError:
        pass
    else:
        raise AssertionError("invalid context accepted")
sc.sc_start(1, sc.SC_NS)
assert cppyy.addressof(sc.sc_find_object("lookup.signal")) == cppyy.addressof(signal)
sc.sc_stop()
print("OBJECT_LOOKUP_REFERENCE 1 1 1 1 1")
