"""Check VCD creation errors and trace recovery against native SystemC."""

from amal.eda.systemc import sc_core as sc
import cppyy


try:
    with sc.vcd_trace("missing_parent/trace"):
        raise AssertionError("trace context entered for a missing directory")
except RuntimeError as error:
    assert "directory does not exist" in str(error)
else:
    raise AssertionError("missing trace directory was accepted")

entered_empty_trace = False
try:
    with sc.vcd_trace(""):
        entered_empty_trace = True
except cppyy.gbl.sc_core.sc_report as report:
    expected_type = cppyy.gbl.sc_core.SC_ID_TRACING_FOPEN_FAILED_
    assert report.get_msg_type() == expected_type
    assert report.get_id() > 0
    print(f"VCD_CREATION_ERROR {report.get_id()}")
else:
    raise AssertionError("empty VCD filename unexpectedly returned a trace file")
assert not entered_empty_trace

signal = sc.sc_signal[int]("creation_signal")
with sc.vcd_trace("creation_recovery", time_unit=(1, sc.SC_NS)) as trace:
    sc.sc_trace(trace, signal, "signal")
    sc.sc_start(1, sc.SC_NS)
    assert signal.read() == 0
    signal.write(7)
    sc.sc_start(1, sc.SC_NS)
    assert signal.read() == 7
    assert sc.sc_time_stamp() == sc.sc_time(2, sc.SC_NS)

print("VCD_CREATION_RECOVERY 2000")
