"""Verify Python owning contexts against native stack copy/restoration semantics."""

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
c = sc_dt.sc_fxtype_context(p, sc_dt.SC_LATER)
baseline = sc_dt.sc_fxtype_params().wl()
p.wl(19)
assert c.value().wl() == 17
assert sc_dt.sc_fxtype_params().wl() == baseline
c.begin()
assert sc_dt.sc_fxtype_params().wl() == 17
nested = sc_dt.sc_context[sc_dt.sc_fxtype_params](sc_dt.sc_fxtype_params(23, 7))
assert sc_dt.sc_fxtype_params().wl() == 23
nested.__destruct__()
assert sc_dt.sc_fxtype_params().wl() == 17
c.end()
assert sc_dt.sc_fxtype_params().wl() == baseline
c.begin()
c.__destruct__()
assert sc_dt.sc_fxtype_params().wl() == baseline
inactive = sc_dt.sc_fxtype_context(sc_dt.sc_fxtype_params(29, 9), sc_dt.SC_LATER)
inactive.__destruct__()
assert sc_dt.sc_fxtype_params().wl() == baseline
print("NATIVE_FX_CONTEXT_AUDIT_OK")

cast = sc_dt.sc_fxcast_context(sc_dt.sc_fxcast_switch(sc_dt.SC_OFF), sc_dt.SC_LATER)
assert cast.value() == sc_dt.sc_fxcast_switch(sc_dt.SC_OFF)
cast.begin()
assert sc_dt.sc_fxcast_switch() == sc_dt.sc_fxcast_switch(sc_dt.SC_OFF)
cast.end()
cast.__destruct__()
for method in ("end", "begin"):
    owner = sc_dt.sc_fxtype_context(sc_dt.sc_fxtype_params(31, 11), sc_dt.SC_LATER)
    if method == "begin":
        owner.begin()
    try:
        getattr(owner, method)()
    except Exception as error:
        assert "context" in str(error).lower(), str(error)
    else:
        raise AssertionError("missing native context report")
    if method == "begin":
        owner.end()
    owner.__destruct__()
assert sc_dt.sc_fxtype_params().wl() == baseline
print("FX_CONTEXT_LIFETIME_OK")
