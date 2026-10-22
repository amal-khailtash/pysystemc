"""Verify public native event naming and borrowed parent metadata."""

from amal.eda.systemc import sc_core as sc
import cppyy

outside = sc.sc_event("outside")
assert outside.name() == "outside" and outside.basename() == "outside"
assert outside.in_hierarchy() and not outside.get_parent_object()
default = sc.sc_event()
assert default.in_hierarchy() and default.name().startswith("event_")
assert not default.get_parent_object()
none = sc.sc_event.none
assert not none.triggered()
for cls, union in ((sc.sc_event_or_list, "or"), (sc.sc_event_and_list, "and")):
    source = cls(outside)
    expression = default | source if union == "or" else default & source
    combined = cls(expression)
    assert combined.size() == 2 and source.size() == 1
    direct = cls(outside | default if union == "or" else outside & default)
    assert direct.size() == 2
pending = sc.sc_event("pending")
pending.notify(sc.sc_time(1, sc.SC_NS))
pending.__destruct__()
name = sc.sc_module_name("owner")
owner = sc.sc_module(name)
inside = sc.sc_event("inside")
assert inside.name() == "owner.inside" and inside.basename() == "inside"
assert inside.in_hierarchy()
assert cppyy.addressof(inside.get_parent_object()) == cppyy.addressof(owner)
del name
snapshot = str(inside.name())
inside.__destruct__()
assert snapshot == "owner.inside" and owner.name() == "owner"
print("EVENT_METADATA_REFERENCE", outside.name(), outside.basename(), snapshot, owner.name())
