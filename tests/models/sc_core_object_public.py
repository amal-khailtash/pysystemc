"""Verify the complete sc_object declaration family through cppyy."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_core_object_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.sc_core_object_probe

assert sc.sc_object.__cpp_name__ == "sc_core::sc_object"
assert sc.sc_object.phase_cb_mask.__cpp_name__ == "sc_core::sc_object::phase_cb_mask"
tree = probe.ObjectTreeProbe("object_public")
first = tree.first
second = tree.second
assert first.name() == "object_public.first"
assert first.basename() == "first"
assert first.kind() == "sc_object"
assert first.get_parent_object() and probe.same_object(first.get_parent_object(), tree)
assert probe.same_object(first.get_parent(), tree)
assert not tree.get_parent_object()
assert cppyy.addressof(first.simcontext()) == cppyy.addressof(sc.sc_get_curr_simcontext())
assert [obj.name() for obj in tree.get_child_objects()] == [first.name(), second.name()]
assert len(tree.get_child_events()) == 1
assert probe.same_event(tree.get_child_events()[0], tree.event)

# Exercise both native stream methods and the null trace-file path.
stream = cppyy.gbl.std.ostringstream()
first.print(stream)
assert stream.str() == first.name()
dump_stream = cppyy.gbl.std.ostringstream()
first.dump(dump_stream)
assert dump_stream.str() == f"name = {first.name()}\nkind = sc_object\n"
first.trace(cppyy.nullptr)
assert probe.printed(first) == first.name()
assert probe.dumped(first) == dump_stream.str()

attribute = sc.sc_attribute[int]("public_value", 41)
duplicate = sc.sc_attribute[int]("public_value", 99)
other = sc.sc_attribute[int]("other_value", 7)
assert first.num_attributes() == 0
assert first.add_attribute(attribute)
assert not first.add_attribute(duplicate)
assert first.add_attribute(other)
assert first.num_attributes() == 2
assert cppyy.addressof(first.get_attribute("public_value")) == cppyy.addressof(attribute)
assert cppyy.addressof(probe.const_attribute(first, "public_value")) == cppyy.addressof(attribute)
assert not first.get_attribute("absent") and not probe.const_attribute(first, "absent")
mutable_attributes = first.attr_cltn()
const_attributes = probe.const_attributes(first)
assert mutable_attributes.size() == 2 and const_attributes.size() == 2
assert cppyy.addressof(mutable_attributes) == cppyy.addressof(const_attributes)
removed = first.remove_attribute("public_value")
assert cppyy.addressof(removed) == cppyy.addressof(attribute)
assert not first.remove_attribute("absent") and first.num_attributes() == 1
first.remove_all_attributes()
assert first.num_attributes() == 0 and first.attr_cltn().size() == 0

# Protected native constructors and child-list/phase-mask helpers are reachable
# only through this C++-derived probe. They borrow all supplied native objects.
default_object = probe.ObjectProbe()
named_object = probe.ObjectProbe("named_object")
copied_object = probe.ObjectProbe(first)
assert default_object.name() and named_object.name() == "named_object"
assert copied_object.basename() == "first_0" and copied_object.name() != first.name()
helper_parent = probe.ObjectProbe("helper_parent")
helper_child = probe.ObjectProbe("helper_child")
helper_event = sc.sc_event("helper_event")
assignment_attribute = sc.sc_attribute[int]("assignment", 5)
assert helper_parent.add_attribute(assignment_attribute)
helper_parent.add_child(helper_child)
helper_parent.add_event(helper_event)
assert [obj.name() for obj in helper_parent.get_child_objects()] == [helper_child.name()]
assert len(helper_parent.get_child_events()) == 1
parent_name = helper_parent.name()
child_name = helper_child.name()
assignment_result = helper_parent.assign_from(helper_child)
assert cppyy.addressof(assignment_result) == cppyy.addressof(helper_parent)
assert helper_parent.name() == parent_name and helper_child.name() == child_name
assert helper_parent.num_attributes() == 1 and helper_parent.get_attribute("assignment").value == 5
assert len(helper_parent.get_child_objects()) == 1 and len(helper_parent.get_child_events()) == 1
assert cppyy.addressof(helper_parent.assign_from(helper_parent)) == cppyy.addressof(helper_parent)
assert helper_parent.remove_attribute("assignment").value == 5
assert helper_parent.remove_child(helper_child) and not helper_parent.remove_child(helper_child)
assert helper_parent.remove_event(helper_event) and not helper_parent.remove_event(helper_event)
mask = int(sc.SC_BEFORE_END_OF_ELABORATION)
assert int(helper_parent.register_mask(mask)) == mask
assert int(helper_parent.unregister_mask(mask)) == 0
printed_value = probe.printed(first)
if isinstance(printed_value, bytes):
    printed_value = printed_value.decode()

print(
    "SC_OBJECT_PUBLIC_REFERENCE "
    f"hierarchy={first.name()} parent={tree.name()} children=2 events=1 "
    f"attributes=2,1,0 output='{printed_value}' copy={copied_object.basename()}"
)
tree.__destruct__()
assert not sc.sc_find_object("object_public")
assert not sc.sc_find_object("object_public.first")
