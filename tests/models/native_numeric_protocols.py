"""Verify selected Python numeric protocols against native SystemC values."""

from amal.eda.systemc import sc_dt, tlm


for flag in (False, True):
    assert bool(sc_dt.sc_bit(flag)) is flag
for cls in (sc_dt.sc_fix, sc_dt.sc_fix_fast):
    assert float(cls(-3.75, 16, 8)) == -3.75
for cls in (sc_dt.sc_fxval, sc_dt.sc_fxval_fast):
    assert float(cls(-3.75)) == -3.75
assert int(sc_dt.sc_int[17](-3)) == -3
assert int(sc_dt.sc_uint[17](7)) == 7
assert int(tlm.tlm_phase(tlm.BEGIN_REQ)) == int(tlm.BEGIN_REQ)
print("NATIVE_NUMERIC_PROTOCOLS_OK")
