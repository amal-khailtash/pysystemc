"""Record Python context construction failure and native stack context semantics."""

from amal.eda.systemc import sc_dt
import cppyy

cppyy.cppdef("""
namespace fx_context_audit {
void check() {
 auto before=sc_dt::sc_fxtype_context::default_value();
 sc_dt::sc_fxtype_params p(17,5);
 sc_dt::sc_fxtype_context c(p,sc_dt::SC_LATER);
 p.wl(19);
 if(c.value().wl()!=17) throw std::runtime_error("context copy");
 c.begin();
 if(sc_dt::sc_fxtype_params().wl()!=17) throw std::runtime_error("begin default");
 c.end();
 if(sc_dt::sc_fxtype_params().wl()!=before.wl()) throw std::runtime_error("restore");
}
}
""")
cppyy.gbl.fx_context_audit.check()
p = sc_dt.sc_fxtype_params(17, 5)
try:
    c = sc_dt.sc_fxtype_context(p, sc_dt.SC_LATER)
except TypeError:
    pass
else:
    c.end()
    raise AssertionError("Reaudit context construction: expected recorded TypeError")
print("NATIVE_FX_CONTEXT_AUDIT_OK")
