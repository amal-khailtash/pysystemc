"""Exercise pointer values and Python callback lifetimes for hash callbacks."""

import gc
from pathlib import Path
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy


def _reject_type_error(callback, message):
    """Require an operation to reject a value that cannot match its signature."""
    try:
        callback()
    except TypeError:
        return
    raise AssertionError(message)


cppyy.include(str(Path(__file__).resolve().parents[1] / "native/hash_callback_pointers_probe.h"))
cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_hash_base_probe.h"))
probe = cppyy.gbl.hash_callback_pointers_probe
hash_base_probe = cppyy.gbl.sc_core_hash_base_probe
hash_type = sc.sc_phash_base.hash_fn_t
compare_type = sc.sc_phash_base.cmpr_fn_t

assert sc.sc_phash_base.__cpp_name__ == "sc_core::sc_phash_base"
assert hash_type.__cpp_name__ == "pysystemc::callback_pointer<2>"
assert compare_type is sc.CFT
assert "exact native callback pointer" in hash_type.__doc__
assert "exact native callback pointer" in compare_type.__doc__
assert "Native reflected constructors do not implicitly accept this holder" in hash_type.__doc__
assert "Native reflected constructors do not implicitly accept this holder" in compare_type.__doc__

for pointer_type in (hash_type, compare_type):
    empty = pointer_type()
    assert not empty.valid() and empty == pointer_type(empty)
    _reject_type_error(lambda: pointer_type(lambda *_: 0), "Python callable entered a native pointer holder")
    _reject_type_error(lambda: pointer_type.from_function("bad"), "unqualified function name was accepted")

hash_pointer = hash_type.from_function("hash_callback_pointers_probe::overloaded_hash")
compare_pointer = compare_type.from_function("hash_callback_pointers_probe::overloaded_compare")
assert hash_pointer.valid() and compare_pointer.valid()
assert hash_type(hash_pointer) == hash_pointer
assert compare_type(compare_pointer) == compare_pointer
assert probe.exact_hash_pointer(hash_pointer)
assert probe.exact_compare_pointer(compare_pointer)
assert probe.exercise_native_table(hash_pointer, compare_pointer)
_reject_type_error(
    lambda: hash_base_probe.BaseProbe(hash_base_probe.null_pointer(), 4, 2, 2.0, False, hash_pointer, compare_pointer),
    "native hash constructor accepted pointer holders through implicit conversion",
)

_reject_type_error(
    lambda: hash_type.from_function("hash_callback_pointers_probe::wrong_hash"),
    "hash function with the wrong signature was accepted",
)
_reject_type_error(
    lambda: compare_type.from_function("hash_callback_pointers_probe::wrong_compare"),
    "comparison function with the wrong signature was accepted",
)

table = sc.sc_phash_base()
table.set_hash_fn(hash_pointer)
table.set_cmpr_fn(compare_pointer)
assert probe.exercise_configured_table(table)

for method, pointer_type in ((table.set_hash_fn, hash_type), (table.set_cmpr_fn, compare_type)):
    try:
        method(pointer_type())
    except ValueError as error:
        assert "null native callback pointer" in str(error)
    else:
        raise AssertionError("native hash setter accepted a null callback holder")


def make_python_callbacks():
    """Create observable Python callbacks accepted by the original hash setters."""
    hash_calls = []
    compare_calls = []

    def hash_key(_pointer):
        """Record calls made through the active Python hash callback."""
        hash_calls.append(True)
        return 0

    def compare_keys(_left, _right):
        """Record calls made through the active Python comparison callback."""
        compare_calls.append(True)
        return 0

    return hash_key, compare_keys, hash_calls, compare_calls


callback_table = sc.sc_phash_base()
old_hash, old_compare, _, _ = make_python_callbacks()
old_hash_ref = weakref.ref(old_hash)
old_compare_ref = weakref.ref(old_compare)
callback_table.set_hash_fn(old_hash)
callback_table.set_cmpr_fn(old_compare)
current_hash, current_compare, current_hash_calls, current_compare_calls = make_python_callbacks()
current_hash_ref = weakref.ref(current_hash)
current_compare_ref = weakref.ref(current_compare)
callback_table.set_hash_fn(current_hash)
callback_table.set_cmpr_fn(current_compare)
del old_hash, old_compare, current_hash, current_compare
gc.collect()
assert old_hash_ref() is None and old_compare_ref() is None
assert current_hash_ref() is not None and current_compare_ref() is not None
owners_before_rejection = callback_table._hash_callback_owners
try:
    callback_table.set_hash_fn(compare_pointer)
except TypeError:
    pass
else:
    raise AssertionError("hash setter accepted a comparison pointer holder")
try:
    callback_table.set_cmpr_fn(hash_pointer)
except TypeError:
    pass
else:
    raise AssertionError("comparison setter accepted a hash pointer holder")
assert callback_table.count() == 0
assert callback_table._hash_callback_owners is owners_before_rejection
assert callback_table._hash_callback_owners["set_hash_fn"] is current_hash_ref()
assert callback_table._hash_callback_owners["set_cmpr_fn"] is current_compare_ref()
null_key = hash_base_probe.null_pointer()
assert callback_table.insert(null_key) == 0
assert callback_table.insert(null_key) == 1
assert callback_table.count() == 1 and current_hash_calls and current_compare_calls
del owners_before_rejection
del callback_table
gc.collect()
assert current_hash_ref() is None and current_compare_ref() is None

QualifiedHash = sc.sc_phash["void*", "const sc_core::sc_event*"]
qualified_table = QualifiedHash()
assert QualifiedHash.__dict__.get("_hash_pointer_setters_wrapped") is True
qualified_table.set_hash_fn(hash_pointer)
qualified_table.set_cmpr_fn(compare_pointer)

print("SC_HASH_CALLBACK_POINTERS exact-overloads table-insert duplicate-update count=1")
