"""Verify unsigned owning shifts and native width semantics."""

from amal.eda.systemc import sc_dt
import cppyy

cppyy.cppdef("""
namespace unsigned_width_audit {
void check() {
 sc_dt::sc_unsigned a(8); a=12;
 auto l=a<<2; auto r=a>>2;
 if(l.length()!=11 || l.to_uint()!=48 || r.length()!=9 || r.to_uint()!=3)
  throw std::runtime_error("nonzero widths");
 a<<=2;
 if(a.length()!=8 || a.to_uint()!=48) throw std::runtime_error("inplace left");
 a>>=2;
 if(a.length()!=8 || a.to_uint()!=12) throw std::runtime_error("inplace right");
 a=0;
 if((a<<2).length()!=8 || (a>>2).length()!=8) throw std::runtime_error("zero copy");
 a=12;
 if((a<<0).length()!=8 || (a>>0).length()!=8) throw std::runtime_error("zero count");
}
}
""")
cppyy.gbl.unsigned_width_audit.check()
a = sc_dt.sc_unsigned(8)
a.__assign__(12)
left = a << 2
right = a >> 2
assert left.length() == 11 and left.to_uint() == 48
assert right.length() == 9 and right.to_uint() == 3
assert a.length() == 8 and a.to_uint() == 12
for count in (0, -1):
    assert (a << count).length() == 8 and (a >> count).to_uint() == 12
a.__assign__(0)
assert (a << 2).length() == 8 and (a >> 2).length() == 8
a.__assign__(12)
left = a.__ilshift__(2)
assert left.length() == 8 and left.to_uint() == 48
right = a.__irshift__(2)
assert right.length() == 8 and right.to_uint() == 12
print("NATIVE_UNSIGNED_SHIFTS_AUDIT_OK")
