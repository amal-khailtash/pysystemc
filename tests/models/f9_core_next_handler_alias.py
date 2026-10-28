"""Check native report signature and its public pointer-value adapter."""

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
assert python_exposed and not cppyy_exposed
assert "sc_report_handler_proc" in sc.__all__
assert sc.sc_report_handler_proc is cppyy.gbl.pysystemc.report_handler_pointer
assert not sc.sc_report_handler_proc().valid()
print("F9_CORE_NEXT_HANDLER_ALIAS 1 1 0")
