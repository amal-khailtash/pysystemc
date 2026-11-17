"""Exercise safe public pointer-vector conversion failure and recovery."""

from amal.eda.systemc import sc_core as sc
import cppyy


TimeVector = sc.sc_pvector["sc_core::sc_time*"]
values = TimeVector()
first = sc.sc_time(17, sc.SC_NS)
replacement = sc.sc_time(23, sc.SC_NS)
wrong_type = sc.sc_event("not_a_time")
values[0] = first
initial_address = cppyy.addressof(values.fetch(0))
assert initial_address == cppyy.addressof(first)


def reject_wrong_pointer(index):
    """Preserve native storage when public pointer conversion is rejected."""
    try:
        values[index] = wrong_type
    except TypeError:
        pass
    else:
        raise AssertionError(f"sc_pvector accepted an sc_event* for slot {index}")
    assert values.size() == 1
    assert cppyy.addressof(values.fetch(0)) == initial_address


reject_wrong_pointer(0)
reject_wrong_pointer(3)
try:
    values.fetch(1)
except IndexError:
    pass
else:
    raise AssertionError("failed pointer conversion grew the vector")

values[0] = replacement
assert values.size() == 1
assert cppyy.addressof(values.fetch(0)) == cppyy.addressof(replacement)

last = sc.sc_time(31, sc.SC_NS)
values[2] = last
assert values.size() == 3
assert not values[1]
assert cppyy.addressof(values.fetch(2)) == cppyy.addressof(last)
try:
    values.fetch(3)
except IndexError:
    pass
else:
    raise AssertionError("fetch grew the pointer vector")
assert values.size() == 3

print("PVECTOR_POINTER_RECOVERY_OK unchanged=2 recovered=1 null_growth=1 fetch=bounded")
