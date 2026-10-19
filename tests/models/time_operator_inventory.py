"""Verify Python protocol spellings for native sc_time member operators."""

from amal.eda.systemc import sc_core as sc

left = sc.sc_time(6, sc.SC_NS)
right = sc.sc_time(2, sc.SC_NS)
context = sc.sc_get_curr_simcontext()
assert sc.sc_time().value() == 0
assert sc.sc_time(left) == left
assert sc.sc_time(6, "ns") == left
assert sc.sc_time(6, "SC_NS", context) == left
assert sc.sc_time(6, sc.SC_NS, context) == left
assert sc.sc_time(6.0, False).value() == 6
assert sc.sc_time(6, False).value() == 6
assert left > right and left >= right and right < left and right <= left
assert left != right and left == sc.sc_time(6, sc.SC_NS)
left.__assign__(right)
assert left == right
left += right
assert left == sc.sc_time(4, sc.SC_NS)
left -= right
assert left == right
left *= 3
assert left == sc.sc_time(6, sc.SC_NS)
left /= 2
assert left == sc.sc_time(3, sc.SC_NS)
left %= right
assert left == sc.sc_time(1, sc.SC_NS)
print("TIME_OPERATOR_MAPPING_OK")
