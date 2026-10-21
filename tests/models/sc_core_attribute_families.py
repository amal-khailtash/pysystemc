"""Verify complete native attribute/base/collection APIs and borrowed ownership."""

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy

base = sc.sc_attr_base("base")
base_copy = sc.sc_attr_base(base)
assert base_copy.name() == "base"
for value_type, value in (
    (int, 17),
    ("std::string", cppyy.gbl.std.string("text")),
    (
        dt.sc_biguint[137],
        dt.sc_biguint[137](29),
    ),
):
    template = sc.sc_attribute[value_type]
    default = template("default")
    assert default.value == value_type() if not isinstance(value_type, str) else default.value == ""
    attribute = template("value", value)
    copied = template(attribute)
    assert copied.name() == "value" and copied.value == value
    attribute.value = value
    if value_type is int:
        attribute.value = 23
        assert attribute.value == 23 and copied.value == 17
    copied.__destruct__()
    default.__destruct__()
    attribute.__destruct__()
collection = sc.sc_attr_cltn()
assert collection.begin() == collection.end()
assert cppyy.gbl.sc_core.sc_attr_cltn.elem_type
assert cppyy.gbl.sc_core.sc_attr_cltn.iterator
assert cppyy.gbl.sc_core.sc_attr_cltn.const_iterator
assert cppyy.gbl.sc_core.sc_attr_cltn.elem_type.__cpp_name__ == "sc_core::sc_attr_base*"
first, second, third = (sc.sc_attribute[int](name, index) for index, name in enumerate(("a", "b", "c")))
assert not collection.push_back(cppyy.nullptr)
for item in (first, second, third):
    assert collection.push_back(item)
assert not collection.push_back(first)
duplicate = sc.sc_attribute[int]("a", 9)
assert not collection.push_back(duplicate)
try:
    collection.push_back(object())
except TypeError:
    pass
else:
    raise AssertionError("invalid attribute accepted")
assert collection.size() == 3 and not collection["missing"]
assert cppyy.addressof(collection["a"]) == cppyy.addressof(first)
copy = sc.sc_attr_cltn(collection)
assert cppyy.addressof(copy["a"]) == cppyy.addressof(first)
iterator, end = collection.begin(), collection.end()
names = []
while iterator != end:
    names.append(iterator.__deref__().name())
    iterator.__preinc__()
assert names == ["a", "b", "c"]
cppyy.cppdef("""
#include <type_traits>
static_assert(std::is_same_v<sc_core::sc_attr_cltn::iterator, std::vector<sc_core::sc_attr_base*>::iterator>);
static_assert(std::is_same_v<sc_core::sc_attr_cltn::const_iterator, std::vector<sc_core::sc_attr_base*>::const_iterator>);
/** @brief Exercise const lookup/iterator overloads on a borrowed live collection.
 * @details Does not own attributes or mutate collection; GIL held, no callbacks.
 */
bool const_attribute_view(const sc_core::sc_attr_cltn& values, const sc_core::sc_attr_base* first) {
 auto begin=values.begin();auto end=values.end();
 return values["a"]==first && !values["missing"] && begin!=end && *begin==first;
}
""")
assert cppyy.gbl.const_attribute_view(copy, first)
assert collection.remove("a").name() == "a" and not collection.remove("missing")
assert collection.begin().__deref__().name() == "c", "native removal swaps final pointer into removed slot"
assert copy.size() == 3 and collection.size() == 2
collection.remove_all()
assert collection.size() == 0 and first.value == 0
copy.__destruct__()
assert first.value == 0 and second.value == 1 and third.value == 2
collection.__destruct__()
base_copy.__destruct__()
base.__destruct__()
print("ATTRIBUTE_FAMILY_REFERENCE base 17 text 29 a b c 3 2 0 0 1 2")
