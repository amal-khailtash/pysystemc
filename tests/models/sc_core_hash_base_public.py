"""Exercise public native chained-hash base classes through cppyy."""

import gc
from pathlib import Path
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_hash_base_probe.h"))
probe = cppyy.gbl.sc_core_hash_base_probe
HashBase = sc.sc_phash_base
HashIter = sc.sc_phash_base_iter


def make_python_callbacks():
    """Build native function-pointer callbacks that close over observable state."""
    hash_calls = []
    compare_calls = []

    def hash_key(pointer):
        """Hash a borrowed integer key, including a null key."""
        value = probe.read_int(pointer)
        hash_calls.append(value)
        return value & 0xFFFFFFFF

    def compare_keys(left, right):
        """Compare borrowed integer keys by value, including null keys."""
        lhs = probe.read_int(left)
        rhs = probe.read_int(right)
        compare_calls.append((lhs, rhs))
        return (lhs > rhs) - (lhs < rhs)

    return hash_key, compare_keys, hash_calls, compare_calls


def read(pointer):
    """Copy an integer from a borrowed pointer, using -1 for null."""
    return probe.read_int(pointer)


# The public node declaration is forward-only. The pointer remains borrowed,
# and the probe intentionally allows only a null node pointer to be observed.
assert cppyy.gbl.sc_core.sc_phash_elem.__cpp_name__ == "sc_core::sc_phash_elem"
assert probe.is_null_element(probe.null_element())

default_table = HashBase()
assert default_table.empty() and default_table.count() == 0
del default_table
gc.collect()

native_hash = probe.hash_int_value
native_compare = probe.compare_int_value
null_pointer = probe.null_pointer()
normalized = probe.BaseProbe(null_pointer, 4, 2, 2.0, False, native_hash, native_compare)
fallback = probe.BaseProbe(null_pointer, 0, 2, 2.0, False, native_hash, native_compare)
assert normalized.bin_count() == 5 and fallback.bin_count() == 11
del normalized, fallback

null_table = HashBase()
null_table.set_hash_fn(native_hash)
null_table.set_cmpr_fn(native_compare)
assert null_table.insert(null_pointer, null_pointer) == 0
assert null_table.contains(null_pointer)
assert null_table.lookup(null_pointer, probe.contents_output_slot()) == 1
assert read(probe.contents_output()) == -1
assert read(null_table[null_pointer]) == -1
assert null_table.remove(null_pointer) == 1 and null_table.empty()
del null_table

element_table = probe.BaseProbe(null_pointer, 11, 5, 2.0, True, native_hash, native_compare)
assert probe.is_null_element(element_table.find_node(probe.key(21)))
assert element_table.insert(probe.key(21), probe.contents(22)) == 0
borrowed_element = element_table.find_node(probe.key(21))
assert not probe.is_null_element(borrowed_element)
assert borrowed_element.__python_owns__ is False
assert probe.is_null_element(element_table.find_node(probe.key(20)))
del borrowed_element
gc.collect()
assert element_table.contains(probe.key(21))
element_table.erase()
del element_table

hash_fn, compare_fn, hash_calls, compare_calls = make_python_callbacks()
hash_ref = weakref.ref(hash_fn)
compare_ref = weakref.ref(compare_fn)
table = HashBase(probe.contents(9), 4, 2, 2.0, False, hash_fn, compare_fn)
del hash_fn, compare_fn
gc.collect()
assert hash_ref() is not None and compare_ref() is not None
assert table.empty() and table.count() == 0
assert table.insert(probe.key(1)) == 0
assert table.insert(probe.key(2), probe.contents(2)) == 0
assert table.insert(probe.key(2), probe.contents(3)) == 1
assert table.insert_if_not_exists(probe.key(2), probe.contents(8)) == 1
assert table.insert_if_not_exists(probe.key(3)) == 0
assert table.count() == 3 and not table.empty()
assert table.contains(probe.key(1)) and not table.contains(probe.key(20))
assert read(table[probe.key(1)]) == 9 and read(table[probe.key(20)]) == 9
assert table.lookup(probe.key(2), probe.contents_output_slot()) == 1
assert read(probe.contents_output()) == 3
assert table.lookup(probe.key(2), None) == 1
assert table.lookup(probe.key(2), cppyy.nullptr) == 1
assert table.lookup(probe.key(20), None) == 0
assert table.lookup(probe.key(20), probe.contents_output_slot()) == 0
assert read(probe.contents_output()) == 9
assert hash_calls and compare_calls

iterator = HashIter.__new__(HashIter)
HashIter.__init__.__overload__("sc_core::sc_phash_base*")(iterator, table)
assert not iterator.empty()
iterated_keys = set()
while not iterator.empty():
    current_key = read(iterator.key())
    iterated_keys.add(current_key)
    if current_key == 1:
        assert read(iterator.set_contents(probe.contents(10))) == 10
    iterator.__postinc__(0)
assert iterated_keys == {1, 2, 3}
iterator.reset.__overload__("sc_core::sc_phash_base &")(table)
assert not iterator.empty()
iterator.remove()
assert table.count() == 2
iterator.reset.__overload__("sc_core::sc_phash_base*")(table)
while not iterator.empty() and read(iterator.key()) != 2:
    iterator.step()
assert not iterator.empty() and read(iterator.contents()) == 3
iterator.remove()
assert table.count() == 1

assert table.insert(probe.key(4), probe.contents(4)) == 0
assert table.remove_by_contents(probe.contents(4)) == 1
assert table.insert(probe.key(5), probe.contents(5)) == 0
assert table.insert(probe.key(6), probe.contents(6)) == 0
predicate_arg = probe.key(5)


def content_is_argument(content, argument):
    """Remove a value when its integer contents match the borrowed argument."""
    return read(content) == read(argument)


assert table.remove_by_contents(content_is_argument, predicate_arg) == 1
assert table.remove(probe.key(5)) == 0
assert table.remove(probe.key(6)) == 1

output_table = HashBase(probe.contents(9), 11, 5, 2.0, True, native_hash, native_compare)
output_table.insert(probe.key(7), probe.contents(8))
assert output_table.remove(probe.key(7), probe.key_output_slot(), probe.contents_output_slot()) == 1
assert read(probe.key_output()) == 7 and read(probe.contents_output()) == 8
assert output_table.remove(probe.key(7), probe.key_output_slot(), probe.contents_output_slot()) == 0
assert read(probe.key_output()) == -1 and read(probe.contents_output()) == -1

source = HashBase(probe.contents(9), 11, 5, 2.0, True, native_hash, native_compare)
source.insert(probe.key(11), probe.contents(12))
source.insert(probe.key(12), probe.contents(13))
pointer_copy = HashBase()
pointer_copy.copy.__overload__("const sc_core::sc_phash_base*")(source)
assert pointer_copy.count() == 2
assert probe.same_pointer(pointer_copy[probe.key(11)], source[probe.key(11)])
reference_copy = HashBase()
reference_copy.copy.__overload__("const sc_core::sc_phash_base &")(source)
assert reference_copy.count() == 2
assert probe.same_pointer(reference_copy[probe.key(12)], source[probe.key(12)])

owned = HashBase(probe.contents(9), 11, 5, 2.0, True, native_hash, native_compare)
assert owned.insert(probe.key(14), probe.contents(15), probe.duplicate_key) == 0
assert owned.insert_if_not_exists(probe.key(14), probe.contents(16), probe.duplicate_key) == 1
assert owned.insert_if_not_exists(probe.key(15), probe.contents(16), probe.duplicate_key) == 0
duplicate_copy = HashBase(null_pointer, 11, 5, 2.0, True, native_hash, native_compare)
duplicate_copy.copy(owned, probe.duplicate_key, probe.free_key)
assert duplicate_copy.count() == 2 and read(duplicate_copy[probe.key(14)]) == 15
duplicate_copy.copy(owned, probe.duplicate_key, probe.free_key)
assert probe.freed_keys() == 2 and duplicate_copy.count() == 2
owned_iterator = HashIter.__new__(HashIter)
HashIter.__init__.__overload__("sc_core::sc_phash_base &")(owned_iterator, duplicate_copy)
assert not owned_iterator.empty()
owned_iterator.remove(probe.free_key)
assert duplicate_copy.count() == 1
assert duplicate_copy.remove(probe.key(15), probe.free_key) == 1
assert probe.freed_keys() == 4 and duplicate_copy.empty()
assert owned.insert(probe.key(16), probe.contents(17), probe.duplicate_key) == 0
assert owned.insert(probe.key(17), probe.contents(16), probe.duplicate_key) == 0
duplicate_copy.copy(owned, probe.duplicate_key, probe.free_key)
assert duplicate_copy.remove_by_contents(probe.contents(16), probe.free_key) == 2
assert duplicate_copy.remove_by_contents(content_is_argument, probe.key(17), probe.free_key) == 1
duplicate_copy.erase(probe.free_key)
assert probe.freed_keys() == 8 and duplicate_copy.empty()
owned.erase(probe.free_key)
assert probe.freed_keys() == 12 and owned.empty()

assert hash_ref() is not None and compare_ref() is not None
table.erase()
del iterator, table
gc.collect()
assert hash_ref() is None and compare_ref() is None

setter_hash, setter_compare, setter_hash_calls, setter_compare_calls = make_python_callbacks()
setter_hash_ref = weakref.ref(setter_hash)
setter_compare_ref = weakref.ref(setter_compare)
setter_table = HashBase()
setter_table.set_hash_fn(setter_hash)
setter_table.set_cmpr_fn(setter_compare)
del setter_hash, setter_compare
gc.collect()
assert setter_hash_ref() is not None and setter_compare_ref() is not None
assert setter_table.insert(probe.key(23), probe.contents(24)) == 0
assert setter_table.contains(probe.key(23)) and setter_hash_calls and setter_compare_calls
del setter_table
gc.collect()
assert setter_hash_ref() is None and setter_compare_ref() is None
source.erase()
pointer_copy.erase()
reference_copy.erase()
output_table.erase()
print("SC_HASH_BASE_REFERENCE rows=45 counts=0,3,2 copies=2,2 iterator=3 removed=6 freed=12 bins=5,11 elem=null")
