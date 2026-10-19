"""Compare fixed-point integer conversion and independent value copies."""

from amal.eda.systemc import sc_dt
import cppyy

cppyy.cppdef("""
namespace fx_conversion_audit {
void check() {
 sc_dt::sc_fix a(-3.75,16,8);
 sc_dt::sc_fix_fast b(-3.75,16,8);
 if(a.to_int()!=-3 || b.to_int()!=-3)
   throw std::runtime_error("truncation");
 if(a.to_uint64()!=sc_dt::uint64(-3) || b.to_uint64()!=sc_dt::uint64(-3))
   throw std::runtime_error("wrap");
 auto av=a.value(); auto bv=b.value();
 a=2; b=2;
 if(av.to_double()!=-3.75 || bv.to_double()!=-3.75)
   throw std::runtime_error("value copy");
}
}
""")
cppyy.gbl.fx_conversion_audit.check()
for cls in (sc_dt.sc_fix, sc_dt.sc_fix_fast):
    number = cls(-3.75, 16, 8)
    assert number.to_int() == -3
    assert number.to_uint64() == (1 << 64) - 3
    value = number.value()
    number.__assign__(2)
    assert value.to_double() == -3.75
    assert number.to_double() == 2
print("NATIVE_FX_CONVERSIONS_AUDIT_OK")
