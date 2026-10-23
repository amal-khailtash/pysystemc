"""Check unbound export reports before elaboration in an isolated kernel."""

from amal.eda.systemc import sc_core as sc

name = sc.sc_module_name("top")
top = sc.sc_module(name)
export = sc.sc_export[sc.sc_signal_in_if[int]]("empty")
assert not export.get_interface()
try:
    getattr(export, "operator->")()
except sc.sc_report as report:
    print("EXPORT_UNBOUND", report.get_msg_type(), report.get_msg(), sep="\t")
else:
    raise AssertionError("unbound arrow returned")
del name
try:
    sc.sc_start(sc.SC_ZERO_TIME)
except sc.sc_report as report:
    print("EXPORT_ELABORATION", report.get_msg_type(), report.get_msg(), sep="\t")
else:
    raise AssertionError("unbound elaboration returned")
