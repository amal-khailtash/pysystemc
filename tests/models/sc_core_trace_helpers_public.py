"""Exercise native trace creation, formatting, delta control and closure."""

from array import array

from amal.eda.systemc import sc_core as sc
import cppyy

signal = sc.sc_signal[bool]("sample")
files = [sc.sc_create_vcd_trace_file("helpers"), sc.sc_create_wif_trace_file("helpers")]
for trace in files:
    trace.set_time_unit(1, sc.SC_PS)
    sc.sc_trace(trace, signal, "sample")
    sc.sc_trace_delta_cycles(trace)
    sc.sc_trace_delta_cycles(trace, False)
    sc.sc_trace_delta_cycles(trace, True)
    sc.sc_write_comment(trace, "plain comment")
    sc.tprintf(trace, "formatted %s %d %u %.2f %%", "word", -17, 29, 1.25)
    sc.tprintf(trace, "lengths %lld %zu %08x %c", 2**40, 137, 255, "Q")
    sc.tprintf(trace, "dynamic %*.*f", 8, 3, 1.25)
    sc.tprintf(trace, "hexfloat %a", 1.25)
    sc.tprintf(trace, "types %hhd %hd %ld %jd %td %.2Lf %ls %lc %p", -3, -7, 23, 2**40, 17, 1.25, "wide", "W", None)
    sc.tprintf(trace, "no arguments %%")
    sc.tprintf(trace, "position %2$d %1$s", "word", 17)
    sc.tprintf(trace, "position dynamic %3$*1$.*2$f", 8, 3, 1.25)
    count = array("i", [0])
    sc.tprintf(trace, "counted%n", count)
    assert count[0] == 7
    sc.tprintf(trace, "count value %d", count[0])
    sc.tprintf(trace, "long %s", "a" * 5000)
# Preserve native null semantics, including the Python None close spelling.
for null in (None, cppyy.nullptr):
    sc.sc_close_vcd_trace_file(null)
    sc.sc_close_wif_trace_file(null)
sc.sc_trace_delta_cycles(cppyy.nullptr)
sc.sc_write_comment(cppyy.nullptr, "ignored")
sc.tprintf(None, "%s", "ignored")
null_count = array("i", [0])
sc.tprintf(None, "null counted%n", null_count)
assert null_count[0] == 12
for fmt, args, error in [
    ("%d", (), TypeError),
    ("text", (1,), TypeError),
    ("%n", (1,), TypeError),
    ("%Ld", (1,), ValueError),
    ("%2$d %d", (1, 2), ValueError),
    ("%c", ("ab",), TypeError),
]:
    try:
        sc.tprintf(files[0], fmt, *args)
    except error:
        pass
    else:
        raise AssertionError(f"invalid format accepted: {fmt}")
signal.write(False)
sc.sc_start(1, sc.SC_NS)
signal.write(True)
sc.sc_start(sc.SC_ZERO_TIME)
signal.write(False)
sc.sc_start(1, sc.SC_NS)
signal.write(True)
sc.sc_start(1, sc.SC_NS)
sc.sc_close_vcd_trace_file(files[0])
sc.sc_close_wif_trace_file(files[1])
assert sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
print("TRACE_HELPERS_REFERENCE 3 ns")
