"""Verify Python protocol spellings for native sc_time member operators."""

from amal.eda.systemc import sc_core as sc

left = sc.sc_time(6, sc.SC_NS)
right = sc.sc_time(2, sc.SC_NS)
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
