"""Verify native name-generator state, copied results and invalid-name recovery."""

from amal.eda.systemc import sc_core as sc

names = sc.sc_name_gen()
other = sc.sc_name_gen()
results = [
    names.gen_unique_name("item"),
    names.gen_unique_name("item"),
    names.gen_unique_name("first", True),
    names.gen_unique_name("first", True),
    other.gen_unique_name("item"),
]
assert results == ["item_0", "item_1", "first", "first_1", "item_0"], results
assert results[0] == "item_0", "returned name retained mutable native buffer"
try:
    names.gen_unique_name("")
except Exception as error:
    assert "unique name" in str(error)
else:
    raise AssertionError("empty name accepted")
assert names.gen_unique_name("item") == "item_2"
print("NAME_GENERATOR_REFERENCE", *results, "item_2")
