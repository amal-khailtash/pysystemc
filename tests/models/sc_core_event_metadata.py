"""Verify public native event naming and borrowed parent metadata."""

from amal.eda.systemc import sc_core as sc
import cppyy

outside = sc.sc_event("outside")
assert outside.name() == "outside" and outside.basename() == "outside"
assert outside.in_hierarchy() and not outside.get_parent_object()
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
