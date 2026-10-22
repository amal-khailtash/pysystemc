"""Validate native WIF and extended VCD trace declarations and transitions."""

import os
from pathlib import Path

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy

os.environ["SC_VCD_SCOPES"] = "ENABLE"
cppyy.cppdef("namespace trace_probe { int signed_value = -17; void advance() { signed_value = -33; } }")
logic = sc.sc_signal_resolved("resolved")
fixed = dt.sc_fixed[16, 8](1.25)
fast = dt.sc_fixed_fast[16, 8](2.5)
time = sc.sc_time(3, sc.SC_NS)
event = sc.sc_event("event")
vcd = sc.sc_create_vcd_trace_file("extended")
wif = sc.sc_create_wif_trace_file("extended")
try:
    for trace in (vcd, wif):
        # Leave room for VCD pseudo-timestamps between nanosecond steps.
        trace.set_time_unit(1, sc.SC_PS if trace is vcd else sc.SC_NS)
        sc.sc_trace(trace, logic, "top.child.resolved")
        sc.sc_trace(trace, fixed, "fixed")
        sc.sc_trace(trace, fast, "fast")
    sc.sc_trace(vcd, time, "time")
    sc.sc_trace(vcd, event, "event")
    cppyy.cppdef("""
    void trace_signed_scalar(sc_core::sc_trace_file* file) {
        sc_core::sc_trace(file, trace_probe::signed_value, "signed_scalar", 8);
    }
    """)
    cppyy.gbl.trace_signed_scalar(vcd)
    vcd.delta_cycles(True)
    logic.write(dt.sc_logic("0"))
    sc.sc_start(1, sc.SC_NS)
    logic.write(dt.sc_logic("1"))
    cppyy.gbl.trace_probe.advance()
    fixed.__assign__(3.5)
    fast.__assign__(4.75)
    event.notify(sc.SC_ZERO_TIME)
    sc.sc_start(1, sc.SC_NS)
    sc.sc_stop()
finally:
    sc.sc_close_vcd_trace_file(vcd)
    sc.sc_close_wif_trace_file(wif)
text = Path("extended.vcd").read_text()
for name in ("resolved", "fixed", "fast", "time", "event", "signed_scalar"):
    assert any(name in line for line in text.splitlines() if line.startswith("$var")), text
assert "$scope module top $end" in text and "$scope module child $end" in text, text
assert "$timescale\n     1 ps\n$end" in text, text
one_ns = text.split("#1000\n", 1)[1].split("\n#", 1)[0]
assert "b11101111 aaaaf" in text, text
assert "1aaaaa" in one_ns and "b11011111 aaaaf" in one_ns, text
assert "\n#2000\n" in text, text
text = Path("extended.awif").read_text()
for name in ("resolved", "fixed", "fast"):
    assert name in text, text
assert "assign" in text, text
print("TRACE_EXTENSIONS_OK")
