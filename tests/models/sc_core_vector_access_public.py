"""Exercise direct and member sc_vector policies with arbitrary native elements."""

from amal.eda.systemc import sc_core as sc
import cppyy
from pathlib import Path

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_vector_access_probe.h"))
probe = cppyy.gbl.sc_core_vector_access_probe
Element = probe.Element
probe.reset_destroyed_elements()

direct_type = sc.sc_direct_access[Element]
const_direct_type = sc.sc_direct_access["const sc_core_vector_access_probe::Element"]
member_type = sc.sc_member_access[Element, int]
const_member_type = member_type.const_policy
non_const_member_type = member_type.non_const_policy

direct = direct_type()
direct_copy = direct_type(direct)
const_direct = const_direct_type(direct)
member = member_type("value")
member_copy = member_type(member)
member_alias_copy = non_const_member_type(member)
const_member = const_member_type(member)
direct_from_member = direct_type(member)
const_direct_from_member = const_direct_type(member)

vector = sc.sc_vector[Element]("python_vector", 3)
for index, (value, identity) in enumerate(((11, 101), (22, 202), (33, 303))):
    vector[index].value = value
    vector[index].identity = identity

direct_element = vector[1]
assert probe.same_element(direct.get(direct_element), direct_element)
assert probe.same_element(direct_copy.get(direct_element), direct_element)
assert probe.same_element(const_direct.get(direct_element), direct_element)
assert probe.same_element(direct_from_member.get(direct_element), direct_element)
assert probe.same_element(const_direct_from_member.get(direct_element), direct_element)
direct_identity = direct.get(direct_element).identity

assert probe.same_member(member.get(direct_element), direct_element)
assert probe.same_member(member_copy.get(direct_element), direct_element)
assert probe.same_member(member_alias_copy.get(direct_element), direct_element)
assert probe.same_member(const_member.get(direct_element), direct_element)
member_value = probe.read_member(member.get(direct_element))

direct_begin = vector.begin()
direct_end = vector.end()
const_direct_iterator_type = sc.sc_vector_iter["const sc_core_vector_access_probe::Element"]
default_direct_iterator = sc.sc_vector_iter[Element]()
const_direct_begin = const_direct_iterator_type(direct_begin)
assert const_direct_begin == direct_begin
assert const_direct_begin[1].identity == 202
assert default_direct_iterator != direct_begin
assert direct_begin != direct_end
direct_pointer = direct_begin.__follow__()
const_direct_pointer = const_direct_begin.__follow__()
assert probe.same_element(direct_pointer, vector[0])
assert probe.same_element(const_direct_pointer, vector[0])
assert direct_pointer.identity == const_direct_pointer.identity == 101

view = vector.assemble("value")
begin, end = view.begin(), view.end()
cursor = begin
cursor.__postinc__(0)
assert cursor.__deref__() == 22
cursor.__preinc__()
assert cursor.__deref__() == 33
assert cursor == end - 1
cursor.__postdec__(0)
assert cursor.__deref__() == 22
assert cursor.__deref__() == 22
cursor.__predec__()
assert cursor.__deref__() == 11
assert cursor == begin
plus_two = begin + 2
assert plus_two.__deref__() == 33
minus_two = end - 2
assert minus_two.__deref__() == 22

advanced = begin + 0
advanced.__iadd__(2)
assert advanced.__deref__() == 33
advanced.__isub__(1)
assert advanced.__deref__() == 22
assert begin[2] == 33
member_direct_begin = const_direct_iterator_type(begin)
member_direct_end = const_direct_iterator_type(end)
assert begin == member_direct_begin
assert begin != member_direct_end
assert begin < member_direct_end and begin <= member_direct_end
assert end > member_direct_begin and end >= member_direct_begin
assert end - member_direct_begin == 3

const_view_begin = view.cbegin()
const_view_end = view.cend()
assert const_view_begin == begin
assert const_view_end - const_view_begin == 3
assert const_view_begin[1] == 22
member_pointer = begin.__follow__()
const_member_pointer = const_view_begin.__follow__()
assert member_pointer[0] == 11
assert const_member_pointer[0] == 11

del direct_pointer, const_direct_pointer, member_pointer, const_member_pointer
del const_view_begin, const_view_end, cursor, advanced, plus_two, minus_two
del member_direct_begin, member_direct_end, begin, end, view
del direct_begin, direct_end, const_direct_begin, default_direct_iterator, direct_element
del direct, direct_copy, const_direct, direct_from_member, const_direct_from_member
del member, member_copy, member_alias_copy, const_member
del vector
assert probe.destroyed_elements() == 3
print(
    "VECTOR_ACCESS_REFERENCE",
    f"direct={direct_identity}",
    f"member={member_value}",
    "iterator=1",
    "const=1",
    f"destroyed={probe.destroyed_elements()}",
)
