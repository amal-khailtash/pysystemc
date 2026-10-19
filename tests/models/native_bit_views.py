"""Check native vector proxy widths, live views, conversion and concatenation."""

from amal.eda.systemc import sc_dt

vector = sc_dt.sc_lv_base("1010")
shifted = vector << 2
assert shifted.length() == 6 and shifted.to_string() == "101000"
reversed_view = vector.range(0, 3)
assert reversed_view.to_string() == "0101"
bit = vector.bit(1)
assert bit.to_char() == "1"
vector.__assign__("0000")
assert bit.to_char() == "0" and reversed_view.to_string() == "0000"
wide = sc_dt.sc_lv_base("1" * 64)
assert wide.to_uint64() == 2**64 - 1
# to_ulong uses one sc_digit even when the platform's long has 64 bits.
assert wide.to_ulong() == 2**sc_dt.SC_DIGIT_SIZE - 1

left = sc_dt.sc_lv_base("101")
right = sc_dt.sc_lv_base("01")
joined = sc_dt.concat(left, right)
assert joined.length() == 5 and joined.to_string() == "10101"
right.__assign__("11")
assert joined.to_string() == "10111"
joined.__assign__("01000")
assert left.to_string() == "010" and right.to_string() == "00"
overlap = sc_dt.sc_lv_base("11110000")
overlap.range(5, 2).__assign__(overlap.range(7, 4))
assert overlap.to_string() == "11111100"
print("NATIVE_BIT_VIEWS_OK")
