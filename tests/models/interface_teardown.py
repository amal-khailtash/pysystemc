"""Verify native borrower rejection and explicit adapter destruction after stop."""

from amal.eda.systemc import sc_core as sc, tlm
import cppyy
import gc
import weakref


class Owner:
    """Provide an analysis callback retained by its structural adapter."""

    def write(self, value):
        """Accept copied native input without retaining it."""
        assert value == 17


name = sc.sc_module_name("top")
top = sc.sc_module(name)
owner = Owner()
reference = weakref.ref(owner)
port = sc.sc_port[tlm.tlm_analysis_if[int]]()
port(owner)
export = sc.sc_export[tlm.tlm_analysis_if[int]]()
export(owner)
cppyy.cppdef("""
/** Native borrower constructed without Python bind interception; caller owns deletion. */
struct TeardownBorrower : sc_core::sc_module {
  sc_core::sc_port<tlm::tlm_analysis_if<int>> port;
  explicit TeardownBorrower(sc_core::sc_module_name name) : sc_module(name), port("port") {}
};
""")
native_name = sc.sc_module_name("native_borrower")
native_borrower = cppyy.gbl.TeardownBorrower(native_name)
native_borrower.port.bind(port.get_interface())
del native_name
native = port.get_interface()
adapter_name = native.name()
native.write(17)
del name, owner
try:
    sc.teardown_interfaces()
except RuntimeError:
    pass
else:
    raise AssertionError("pre-stop deletion accepted")
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
try:
    sc.teardown_interfaces()
except RuntimeError:
    pass
else:
    raise AssertionError("borrowed adapter deletion accepted")
assert reference() is not None
native.write(17)
port.__destruct__()
for borrower in (export, native_borrower):
    try:
        sc.teardown_interfaces()
    except RuntimeError:
        pass
    else:
        raise AssertionError("remaining native/export borrower ignored")
    borrower.__destruct__()
assert sc.teardown_interfaces() == 1
assert not cppyy.gbl.sc_core.sc_find_object(adapter_name)
gc.collect()
assert reference() is None
assert sc.teardown_interfaces() == 0
print("INTERFACE_TEARDOWN_OK 1 1 ns")
