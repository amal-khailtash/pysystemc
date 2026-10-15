"""Exercise documented argument errors without corrupting native kernel state."""

from amal.eda.systemc import sc_core as sc, print_hierarchy, print_module_hierarchy

for call in (lambda: sc.sc_spawn(None), lambda: sc.set_report_handler(42)):
    try:
        call()
    except TypeError:
        pass
    else:
        raise AssertionError("invalid callback accepted")
sc.set_report_handler(lambda report, actions: None)
sc.set_report_handler()
assert sc.ScStatus.from_cpp_enum(sc.SC_ELABORATION) == sc.ScStatus.SC_ELABORATION
assert str(sc.ScStatus.SC_ELABORATION) == "SC_ELABORATION"
print_hierarchy()
signal = sc.sc_signal[bool]("non_module")
print_module_hierarchy(signal)
try:
    with sc.vcd_trace("bad_units", time_unit=(1,)):
        raise AssertionError("invalid time unit accepted")
except TypeError:
    pass
with sc.vcd_trace("normal_close"):
    sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
sc.sc_stop()
assert sc.sc_status() == sc.ScStatus.SC_STOPPED
print("API_ERRORS_OK")
