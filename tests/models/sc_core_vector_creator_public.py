"""Verify Python creator ownership transfer and native failure rollback."""

from amal.eda.systemc import sc_core as sc
import cppyy
import gc
from pathlib import Path
import sys
import weakref

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_vector_creator_probe.h"))
Element = cppyy.gbl.vector_creator_probe.Element
Vector = sc.sc_vector[Element]
invalid_return = len(sys.argv) > 1 and sys.argv[1] == "invalid"
vector = Vector("creator_rollback")
weak_elements = []


def failing(name, index):
    """Create two elements, then fail before returning the third owned value."""
    if index == 2:
        if invalid_return:
            return object()
        raise RuntimeError("creator failure")
    element = Element(name, index)
    weak_elements.append(weakref.ref(element))
    return element


try:
    vector.init(4, failing)
except Exception as error:
    expected = "newly allocated native element" if invalid_return else "creator failure"
    assert expected in str(error), str(error)
else:
    raise AssertionError("creator failure was suppressed")
assert vector.size() == 0 and Element.destroyed == 2
assert not vector._vector_element_owners
assert all(reference() is None for reference in weak_elements)


def successful(name, index):
    """Create an arbitrary native value whose ownership transfers to the vector."""
    element = Element(name, index)
    weak_elements.append(weakref.ref(element))
    return element


vector.init(3, successful)
assert vector.size() == 3 and [vector[index].index for index in range(3)] == [0, 1, 2]
assert all(reference() is not None for reference in weak_elements[2:])
for element in vector._vector_element_owners:
    assert not element.__python_owns__
del element
outside = Element("after_creator_failure")
assert not outside.get_parent_object()
del outside
assert Element.destroyed == 3
del vector
gc.collect()
assert Element.destroyed == 6
assert all(reference() is None for reference in weak_elements)
print("VECTOR_CREATOR_OK", 0, 3, Element.destroyed)
