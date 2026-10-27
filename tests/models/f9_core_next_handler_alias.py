"""Check direct lookup status and compile the report handler callback type."""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef(
    """
    #include <type_traits>
    static_assert(std::is_same<sc_core::sc_report_handler_proc,
                               void (*)(const sc_core::sc_report&, const sc_core::sc_actions&)>::value);
    """
)


def alias_is_exposed(namespace):
    """Return whether a namespace resolves the public typedef directly."""
    try:
        getattr(namespace, "sc_report_handler_proc")
    except AttributeError:
        return False
    return True


python_exposed = alias_is_exposed(sc)
cppyy_exposed = alias_is_exposed(cppyy.gbl.sc_core)
assert not python_exposed and not cppyy_exposed
print("F9_CORE_NEXT_HANDLER_ALIAS 1 0 0")
