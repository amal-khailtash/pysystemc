"""Check inherited ``sc_phash_base`` constructor defaults through cppyy."""

import gc
from pathlib import Path
import weakref

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/hash_constructor_defaults_probe.h"))
Probe = cppyy.gbl.hash_constructor_defaults_probe.Probe
ProbeWithTag = cppyy.gbl.hash_constructor_defaults_probe.ProbeWithTag
assert sc.adapt_inherited_hash_constructor(Probe) is Probe
assert sc.adapt_inherited_hash_constructor(Probe) is Probe
assert sc.adapt_inherited_hash_constructor(ProbeWithTag) is ProbeWithTag

try:
    sc.adapt_inherited_hash_constructor(sc.sc_phash_base)
except TypeError as error:
    assert "native subclass" in str(error)
else:
    raise AssertionError("the hash-base adapter accepted the base class itself")


def configuration(table):
    """Read the native table configuration exposed by the probe.

    Args:
        table: A probe object constructed through an inherited native constructor.

    Returns:
        Bucket count, maximum density, growth factor and reorder flag.
    """
    return table.bins(), table.density(), table.growth(), table.reorder()


omitted = Probe()
explicit_five = Probe(cppyy.nullptr, 11, 5, 2.0, True)
native_hash = cppyy.gbl.sc_core.default_ptr_hash_fn
explicit_seven = Probe(cppyy.nullptr, 11, 5, 2.0, True, native_hash, cppyy.nullptr)
custom = Probe(cppyy.nullptr, 17, 3, 1.5, False, native_hash, cppyy.nullptr)

arity_tables = (
    (Probe(cppyy.nullptr), (11, 5, 2.0, True)),
    (Probe(cppyy.nullptr, 13), (13, 5, 2.0, True)),
    (Probe(cppyy.nullptr, 13, 4), (13, 4, 2.0, True)),
    (Probe(cppyy.nullptr, 13, 4, 1.5), (13, 4, 1.5, True)),
    (Probe(cppyy.nullptr, 13, 4, 1.5, False), (13, 4, 1.5, False)),
    (Probe(cppyy.nullptr, 13, 4, 1.5, False, native_hash), (13, 4, 1.5, False)),
    (Probe(cppyy.nullptr, 13, 4, 1.5, False, native_hash, cppyy.nullptr), (13, 4, 1.5, False)),
)
assert all(configuration(table) == expected for table, expected in arity_tables)
keyword_table = Probe(size=15, density=6, grow=3.0, reorder=False)
assert configuration(keyword_table) == (15, 6, 3.0, False)
tagged = ProbeWithTag(23)
assert tagged.tag() == 23

try:
    Probe(cppyy.nullptr, 11, "invalid", 2.0, True, native_hash, cppyy.nullptr)
except TypeError as error:
    assert "takes at least 7 arguments" not in str(error)
else:
    raise AssertionError("the adapter accepted an invalid full constructor call")

selected = Probe.__new__(Probe)
selected_signature = (
    "void*, int, int, double, bool, sc_core::sc_phash_base::hash_fn_t, sc_core::sc_phash_base::cmpr_fn_t"
)
Probe.__init__.__overload__(selected_signature)(selected, cppyy.nullptr, 11, 5, 2.0, True)
try:
    Probe.__init__.__overload__("not a native constructor signature")
except LookupError:
    pass
else:
    raise AssertionError("the adapter changed cppyy's invalid-overload error")


def hash_callback(_key):
    """Return a stable hash for callback lifetime validation."""
    return 0


def compare_callback(_left, _right):
    """Return equality for callback lifetime validation."""
    return 0


hash_ref = weakref.ref(hash_callback)
compare_ref = weakref.ref(compare_callback)
callback_table = Probe(cppyy.nullptr, 11, 5, 2.0, True, hash_callback, compare_callback)
del hash_callback, compare_callback
gc.collect()
assert hash_ref() is not None and compare_ref() is not None
del callback_table
gc.collect()
assert hash_ref() is None and compare_ref() is None

assert configuration(omitted) == (11, 5, 2.0, True)
assert configuration(explicit_five) == (11, 5, 2.0, True)
assert configuration(explicit_seven) == (11, 5, 2.0, True)
assert configuration(selected) == (11, 5, 2.0, True)
assert configuration(custom) == (17, 3, 1.5, False)
assert (
    sc.PHASH_DEFAULT_INIT_TABLE_SIZE,
    sc.PHASH_DEFAULT_MAX_DENSITY,
    sc.PHASH_DEFAULT_GROW_FACTOR,
    sc.PHASH_DEFAULT_REORDER_FLAG,
) == (11, 5, 2.0, True)

print("HASH_DEFAULTS omitted 11 5 2 1")
print("HASH_DEFAULTS explicit 11 5 2 1")
print("HASH_DEFAULTS custom 17 3 1.5 0")
print("HASH_CONSTANTS 11 5 2 1")
