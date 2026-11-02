"""Check inherited ``sc_phash_base`` constructor defaults through cppyy."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/hash_constructor_defaults_probe.h"))
Probe = cppyy.gbl.hash_constructor_defaults_probe.Probe


def configuration(table):
    """Read the native table configuration exposed by the probe.

    Args:
        table: A probe object constructed through an inherited native constructor.

    Returns:
        Bucket count, maximum density, growth factor and reorder flag.
    """
    return table.bins(), table.density(), table.growth(), table.reorder()


omitted = Probe()
try:
    Probe(cppyy.nullptr, 11, 5, 2.0, True)
except TypeError as error:
    assert "takes at least 7 arguments" in str(error)
else:
    raise AssertionError("inherited trailing-default limitation changed")
native_hash = cppyy.gbl.sc_core.default_ptr_hash_fn
explicit = Probe(cppyy.nullptr, 11, 5, 2.0, True, native_hash, cppyy.nullptr)
custom = Probe(cppyy.nullptr, 17, 3, 1.5, False, native_hash, cppyy.nullptr)

assert configuration(omitted) == (11, 5, 2.0, True)
assert configuration(explicit) == (11, 5, 2.0, True)
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
print("HASH_INHERITED_TRAILING_DEFAULTS_UNSUPPORTED")
