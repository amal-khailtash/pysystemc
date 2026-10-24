"""Verify typed SystemC pool allocation, reset, capacity and value lifetimes."""

import gc
from pathlib import Path

import amal.eda.systemc as _systemc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_vpool_probe.h"))
probe = cppyy.gbl.vpool_probe
vpool_template = _systemc.sc_core.sc_vpool
assert vpool_template.__cpp_name__ == "sc_core::sc_vpool"


def verify_pool(name, value_type, fixture_template):
    """Call native pool methods directly and compare typed slot indices."""
    specialization = vpool_template[value_type]
    assert specialization.__cpp_name__.startswith("sc_core::sc_vpool<")
    owner = fixture_template[value_type]()
    pool = owner.pool()
    capacity = int(pool.size())
    stride = int(probe.slot_stride[value_type]())
    alignment = int(probe.slot_alignment[value_type]())
    base = pool.allocate()
    base_address = cppyy.addressof(base)
    offsets = [0]
    offsets.extend((cppyy.addressof(pool.allocate()) - base_address) // stride for _ in range(capacity + 1))
    pool.reset()
    reset_offset = (cppyy.addressof(pool.allocate()) - base_address) // stride
    expected = list(range(capacity)) + [0, 1]
    aligned = base_address % alignment == 0
    assert capacity == 8
    assert aligned
    assert offsets == expected, (name, offsets, expected)
    assert reset_offset == 0
    print(f"VPOOL|{name}|{capacity}|{int(aligned)}|{stride}|{','.join(map(str, offsets))}|{reset_offset}")
    del pool
    del owner
    gc.collect()


allocated_types = (
    ("int", cppyy.gbl.int),
    ("user_slot", probe.user_slot),
    ("sc_unsigned", cppyy.gbl.sc_dt.sc_unsigned),
    ("sc_concatref", cppyy.gbl.sc_dt.sc_concatref),
    ("sc_unsigned_bitref", cppyy.gbl.sc_dt.sc_unsigned_bitref),
    ("sc_uint_subref", cppyy.gbl.sc_dt.sc_uint_subref),
    ("sc_signed_bitref", cppyy.gbl.sc_dt.sc_signed_bitref),
    ("sc_int_bitref", cppyy.gbl.sc_dt.sc_int_bitref),
    ("sc_int_subref", cppyy.gbl.sc_dt.sc_int_subref),
    ("sc_signed_subref", cppyy.gbl.sc_dt.sc_signed_subref),
    ("sc_uint_bitref", cppyy.gbl.sc_dt.sc_uint_bitref),
    ("sc_concat_bool", cppyy.gbl.sc_dt.sc_concat_bool),
    ("sc_unsigned_subref", cppyy.gbl.sc_dt.sc_unsigned_subref),
)
for name, value_type in allocated_types:
    verify_pool(name, value_type, probe.allocated_pool)

probe.reset_lifetime_counts()
verify_pool("lifetime_slot", probe.lifetime_slot, probe.allocated_pool)
allocated_constructors = int(probe.lifetime_constructor_count())
allocated_destructors = int(probe.lifetime_destructor_count())
assert allocated_constructors == 8 and allocated_destructors == 0
print(f"VPOOL_ALLOCATED_LIFETIME|{allocated_constructors}|{allocated_destructors}")

external_types = (
    ("int", cppyy.gbl.int),
    ("user_slot", probe.user_slot),
)
for name, value_type in external_types:
    verify_pool(name, value_type, probe.external_pool)

probe.reset_lifetime_counts()
verify_pool("lifetime_slot", probe.lifetime_slot, probe.external_pool)
external_constructors = int(probe.lifetime_constructor_count())
external_destructors = int(probe.lifetime_destructor_count())
assert external_constructors == external_destructors == 8
print(f"VPOOL_EXTERNAL_LIFETIME|{external_constructors}|{external_destructors}")
