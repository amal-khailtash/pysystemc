"""Verify structural binding owner retention across supported native borrowers."""

import gc
import weakref

from amal.eda.systemc import sc_core as sc, tlm
from amal.eda.systemc.sc_core import _interfaces
import cppyy


class Owner:
    """Provide an analysis callback with observable dispatch ownership."""

    def __init__(self, label, calls):
        """Retain a callback label and a shared observation list."""
        self.label = label
        self.calls = calls

    def write(self, value):
        """Record the callback receiver and native payload."""
        self.calls.append((self.label, value))


class InvalidOwner:
    """Omit the required analysis callback to force pre-bind adaptation failure."""


name = sc.sc_module_name("f6_binding_top")
top = sc.sc_module(name)

adapter_count = len(_interfaces._owners)
invalid = InvalidOwner()
invalid_ref = weakref.ref(invalid)
invalid_port = sc.sc_port[tlm.tlm_analysis_if[int]]("invalid_port")
try:
    invalid_port.bind(invalid)
except TypeError as error:
    assert str(error) == "tlm::tlm_analysis_if requires callable write", str(error)
else:
    raise AssertionError("missing analysis callback unexpectedly adapted")
assert invalid_port.bind_count() == 0
del invalid
gc.collect()
assert invalid_ref() is None
assert len(_interfaces._owners) == adapter_count
invalid_port.__destruct__()

calls = []
owner = Owner("bound", calls)
owner_ref = weakref.ref(owner)
port = sc.sc_port[tlm.tlm_analysis_if[int]]("port")
export = sc.sc_export[tlm.tlm_analysis_if[int]]("export")
port.bind(owner)
export.bind(owner)

port_interface = port.get_interface()
export_interface = export.get_interface()
adapter_address = cppyy.addressof(port_interface)
assert cppyy.addressof(export_interface) == adapter_address
adapter_name = port_interface.name()
assert adapter_name

rejected = Owner("rejected", calls)
rejected_ref = weakref.ref(rejected)
try:
    export.bind(rejected)
except sc.sc_report as report:
    assert report.get_id() == 126, (report.get_id(), report.get_msg())
else:
    raise AssertionError("duplicate export binding unexpectedly succeeded")

assert cppyy.addressof(port.get_interface()) == adapter_address
assert cppyy.addressof(export.get_interface()) == adapter_address
del owner, rejected, name, port_interface, export_interface
gc.collect()
assert owner_ref() is not None
assert rejected_ref() is not None

port.get_interface().write(17)
export.get_interface().write(23)
assert calls == [("bound", 17), ("bound", 23)], calls

sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
for label in ("port-and-export", "export"):
    try:
        sc.teardown_interfaces()
    except RuntimeError:
        pass
    else:
        raise AssertionError(f"interface teardown ignored live {label} borrower")
    if label == "port-and-export":
        port.__destruct__()
    else:
        export.__destruct__()

assert sc.teardown_interfaces() == 2
assert not cppyy.gbl.sc_core.sc_find_object(adapter_name)
gc.collect()
assert owner_ref() is None
assert rejected_ref() is None
assert sc.teardown_interfaces() == 0
print("F6_BINDING_OWNER_OK invalid=unretained reuse=port,export duplicate=126 borrowers=2 released=2")
