"""Exercise explicit native constructor selection for ``sc_phash_base``."""

import gc
from pathlib import Path
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_hash_base_probe.h"))
probe = cppyy.gbl.sc_core_hash_base_probe
HashBase = sc.sc_phash_base

# This is the concrete constructor declaration exposed by the live CPPOverload
# docstring and by SystemC's sc_hash.h declaration.
CONSTRUCTOR_SIGNATURE = (
    "void*, int, int, double, bool, sc_core::sc_phash_base::hash_fn_t, sc_core::sc_phash_base::cmpr_fn_t"
)


def make_python_callbacks():
    """Build Python hash and compare callbacks with observable call records.

    Returns:
        A pair of callbacks and their call-record lists.
    """
    hash_calls = []
    compare_calls = []

    def hash_key(pointer):
        """Hash a borrowed integer key and record the native callback call."""
        value = probe.read_int(pointer)
        hash_calls.append(value)
        return probe.hash_int_value(pointer)

    def compare_keys(left, right):
        """Compare borrowed integer keys and record the native callback call."""
        lhs = probe.read_int(left)
        rhs = probe.read_int(right)
        compare_calls.append((lhs, rhs))
        return probe.compare_int_value(left, right)

    return hash_key, compare_keys, hash_calls, compare_calls


def test_selected_constructor_lifetime():
    """Prove explicit selection invokes and retains callbacks through teardown."""
    selected_constructor = HashBase.__init__.__overload__(CONSTRUCTOR_SIGNATURE)
    table = HashBase.__new__(HashBase)
    hash_fn, compare_fn, hash_calls, compare_calls = make_python_callbacks()
    hash_ref = weakref.ref(hash_fn)
    compare_ref = weakref.ref(compare_fn)

    selected_constructor(table, probe.contents(9), 5, 2, 2.0, False, hash_fn, compare_fn)
    del hash_fn, compare_fn
    gc.collect()
    assert hash_ref() is not None and compare_ref() is not None
    assert table.empty() and table.count() == 0

    # Keys 1 and 6 collide in five bins, requiring both native callbacks.
    assert table.insert(probe.key(1), probe.contents(11)) == 0
    assert table.insert(probe.key(6), probe.contents(22)) == 0
    assert table.count() == 2 and not table.empty()
    assert table.contains(probe.key(1)) and table.contains(probe.key(6))
    assert table.lookup(probe.key(6), probe.contents_output_slot()) == 1
    assert probe.read_int(probe.contents_output()) == 22
    assert probe.read_int(table[probe.key(1)]) == 11
    assert 1 in hash_calls and 6 in hash_calls
    assert compare_calls and {value for pair in compare_calls for value in pair} >= {1, 6}

    del table
    gc.collect()
    assert hash_ref() is None and compare_ref() is None


def test_invalid_signature_then_valid_recovery():
    """Reject an invalid selection and construct a fresh proxy with a valid one."""
    try:
        HashBase.__init__.__overload__("not a native constructor signature")
    except LookupError:
        pass
    else:
        raise AssertionError("an invalid native constructor signature was accepted")
    recovered = HashBase.__new__(HashBase)
    HashBase.__init__.__overload__(CONSTRUCTOR_SIGNATURE)(
        recovered,
        probe.contents(0),
        5,
        2,
        2.0,
        False,
        probe.hash_int_value,
        probe.compare_int_value,
    )
    assert recovered.empty() and recovered.count() == 0
    assert recovered.insert(probe.key(3), probe.contents(23)) == 0
    assert recovered.contains(probe.key(3)) and recovered.count() == 1
    assert probe.read_int(recovered[probe.key(3)]) == 23
    del recovered
    gc.collect()


test_selected_constructor_lifetime()
test_invalid_signature_then_valid_recovery()
print("HASH_CONSTRUCTOR_SELECTION selected=1 callbacks=1 recovered=1")
