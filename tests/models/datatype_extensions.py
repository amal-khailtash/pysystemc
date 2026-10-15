"""Check native concatenation, dynamic widths and fixed-fast arithmetic."""

from amal.eda.systemc import sc_dt as dt

high = dt.sc_uint[4](10)
low = dt.sc_uint[4](3)
combined = dt.concat(high, low)
assert combined.to_uint64() == 0xA3
combined.__assign__(0x5C)
assert high.to_uint() == 5 and low.to_uint() == 12
for width in (7, 73, 193):
    unsigned = dt.sc_unsigned(width)
    unsigned.__assign__("0b" + "1" * width)
    assert unsigned.length() == width
    assert unsigned.range(6, 0).to_uint() == 127
    signed = dt.sc_signed(width)
    signed.__assign__(-17)
    assert signed.to_int64() == -17
bits = dt.sc_bv_base(73)
bits.__assign__("1" * 73)
assert bits.length() == 73 and bits.range(6, 0).to_uint() == 127
logic = dt.sc_lv_base(73)
logic.__assign__("0" * 69 + "01XZ")
assert logic.to_string().endswith("01XZ")
for template in (dt.sc_fixed, dt.sc_fixed_fast):
    left = template[16, 8](1.5)
    right = template[16, 8](2.25)
    assert (left + right).to_double() == 3.75
    assert (left * right).to_double() == 3.375
    left.__assign__(left + right)
    assert left.to_double() == 3.75
carry = dt.sc_fxval("255.75")
assert (carry + dt.sc_fxval("0.5")).to_double() == 256.25
assert (dt.sc_fxval("-255.75") - dt.sc_fxval("0.5")).to_double() == -256.25
print("DATATYPE_EXTENSIONS_OK")
