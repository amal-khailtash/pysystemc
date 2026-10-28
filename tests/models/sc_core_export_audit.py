"""Audit string specialization, borrowed export access, and report rollback."""

import gc

from amal.eda.systemc import sc_core as sc
import cppyy


name = sc.sc_module_name("export_audit_top")
top = sc.sc_module(name)
signal = sc.sc_signal[int]("source")
export_type = sc.sc_export["sc_core::sc_signal_in_if<int>"]
export = export_type("export")
export.bind(signal)
signal_address = cppyy.addressof(signal)

# get_interface and the selected arrow adapter return borrowed native proxies.
interface = export.get_interface()
assert cppyy.addressof(interface) == signal_address
del interface
gc.collect()
interface = getattr(export, "operator->")()
assert cppyy.addressof(interface) == signal_address
signal.write(47)
del name
sc.sc_start(1, sc.SC_NS)
assert interface.read() == 47
del interface
gc.collect()
assert export.get_interface().read() == 47

# A failed second bind must preserve the first borrowed interface.
try:
    export(signal)
except sc.sc_report as report:
    assert report.get_id() == 126, (report.get_id(), report.get_msg())
    error = report.get_msg()
else:
    raise AssertionError("duplicate export binding unexpectedly succeeded")
assert cppyy.addressof(export.get_interface()) == signal_address
assert export.get_interface().read() == 47

assert export.get_interface().read() == 47
sc.sc_stop()
print("EXPORT_AUDIT_OK type=string arrow=borrowed duplicate=126 value=47", error)

export.__destruct__()
