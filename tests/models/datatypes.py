"""Validate on-demand arbitrary-width native datatypes and bit/range access."""
from amal.eda.systemc import sc_core as sc, sc_dt as dt

for width in (1, 4, 17, 63, 64):
    value = dt.sc_uint[width]("0b" + "1" * width)
    assert value.to_uint64() == (1 << width) - 1
    value = dt.sc_uint[width]("0b1" + "0" * width)
    assert value.to_uint64() == 0
    signed = dt.sc_int[width](-1)
    assert signed.to_int64() == -1

signals = []
for width in (65, 137, 257):
    unsigned = dt.sc_biguint[width]("0b1" + "0" * (width - 1))
    assert unsigned.length() == width
    assert unsigned[width - 1].to_bool()
    assert unsigned.range(7, 0).to_uint() == 0
    signed = dt.sc_bigint[width](-1)
    assert signed.to_int64() == -1
    signal = sc.sc_signal[dt.sc_biguint[width]](f"wide_{width}")
    signal.write(unsigned)
    signals.append((signal, unsigned))

sc.sc_start(sc.SC_ZERO_TIME)
for signal, unsigned in signals:
    assert signal.read() == unsigned

bits = dt.sc_bv[137]("0b1010")
assert bits.range(3, 0).to_uint() == 10
logic = dt.sc_lv[17]("01XZ")
assert logic.to_string().endswith("01XZ")
print("DATATYPES_OK")
