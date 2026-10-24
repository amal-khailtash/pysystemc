"""Exercise wide native exports, protected lifecycle hooks and binding reports."""

from pathlib import Path
from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_exports_probe.h"))
p = cppyy.gbl.export_probe
name = sc.sc_module_name("top")
top = sc.sc_module(name)
signal = sc.sc_signal[sc_dt.sc_biguint[137]]("signal")
a = sc.sc_export[p.IF]("a")
b = sc.sc_export[p.IF]()
probe = p.Probe("probe")
default_probe = p.Probe()
a.bind(signal)
b(signal)
probe.bind(signal)
default_probe.bind(signal)
assert cppyy.addressof(a.get_interface()) == cppyy.addressof(signal)
assert a.kind() == "sc_export" and b.basename().startswith("export_")
assert sc.sc_export_base.this_type is sc.sc_export_base
for label, action in (
    ("duplicate", lambda: a.bind(signal)),
    ("probe", lambda: probe.error(True)),
    ("plain", lambda: probe.error(False)),
):
    try:
        action()
    except sc.sc_report as report:
        print("EXPORT_ERROR", label, report.get_msg_type(), report.get_msg(), sep="\t")
    else:
        raise AssertionError(label)
del name
signal.write(sc_dt.sc_biguint[137]("0x10000000000000000000000000000000001"))
sc.sc_start(1, sc.SC_NS)
print("EXPORT_ACCESS", p.access(a), sep="\t")
sc.sc_stop()
assert probe.hooks == "BESF" and default_probe.hooks == "BESF"
print("EXPORT_HOOKS", probe.hooks, default_probe.hooks, sep="\t")
probe.__destruct__()
default_probe.__destruct__()
assert p.Probe.destroyed == 2
print("EXPORT_DESTRUCTION", p.Probe.destroyed, sep="\t")
a.__destruct__()
b.__destruct__()
