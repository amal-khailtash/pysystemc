"""Verify native hash helpers and explicit key allocation ownership."""

from amal.eda.systemc import sc_core as sc
import cppyy
import cppyy.ll

for value in (0, 1, 4, 17, 0x12345678):
    pointer = cppyy.ll.cast["void*"](value)
    integer = sc.default_int_hash_fn(pointer)
    address = sc.default_ptr_hash_fn(pointer)
    assert integer == (value * 3141592661) & 0xFFFFFFFF
    assert address == ((value >> 2) * 2654435789) & 0xFFFFFFFF
    print("HASH_REFERENCE", value, integer, address)
for key in ("", "alpha", "SystemC", "a.b_9"):
    storage = cppyy.gbl.std.string(key)
    pointer = storage.c_str()
    hashed = sc.default_str_hash_fn(pointer)
    duplicate = sc.sc_strhash_kdup(pointer)
    assert duplicate
    assert sc.sc_strhash_cmp(pointer, duplicate) == 0
    assert sc.sc_strhash_cmp(duplicate, pointer) == 0
    assert sc.default_str_hash_fn(duplicate) == hashed
    assert sc.sc_strhash_cmp(duplicate, duplicate) == 0
    sc.sc_strhash_kfree(duplicate)
    assert sc.default_str_hash_fn(key.encode()) == hashed
    print("STRING_HASH_REFERENCE", key.encode().hex() or "empty", hashed)
assert sc.default_str_hash_fn(cppyy.nullptr) == 0
for left, right, expected in (("a", "b", -1), ("same", "same", 0), ("z", "a", 1)):
    a, b = cppyy.gbl.std.string(left), cppyy.gbl.std.string(right)
    result = sc.sc_strhash_cmp(a.c_str(), b.c_str())
    assert (result > 0) - (result < 0) == expected
print("HASH_COMPARE_REFERENCE -1 0 1 0")
for function, args in (
    (sc.default_str_hash_fn, (object(),)),
    (sc.sc_strhash_kdup, (object(),)),
    (sc.sc_strhash_cmp, (object(), "key")),
):
    try:
        function(*args)
    except TypeError:
        pass
    else:
        raise AssertionError("invalid key accepted")
