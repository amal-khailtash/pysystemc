"""Check the native status enum export and the compatibility state query."""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef(
    """
    #include <type_traits>
    namespace f9_status_alias_probe {
    static_assert(std::is_enum<sc_core::sc_status>::value,
                  "SystemC status is a native enum");
    inline sc_core::sc_status echo(sc_core::sc_status value) { return value; }
    }
    """
)

native_status_type = cppyy.gbl.sc_core.sc_status
assert sc.sc_status_type is native_status_type
assert "sc_status_type" in sc.__all__
assert sc.sc_status_type.SC_UNITIALIZED == cppyy.gbl.sc_core.SC_UNITIALIZED
assert cppyy.gbl.f9_status_alias_probe.echo(sc.sc_status_type.SC_UNITIALIZED) == sc.sc_status_type.SC_UNITIALIZED

state = sc.sc_status()
assert isinstance(state, sc.ScStatus)
assert state is sc.ScStatus(state.value)
assert callable(sc.sc_status)
print("F9_CORE_STATUS_ALIAS 1 1 1")
