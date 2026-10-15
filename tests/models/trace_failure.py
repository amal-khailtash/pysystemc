"""Verify trace setup failures close native files and preserve Python errors."""

from pathlib import Path
from amal.eda.systemc import sc_core as sc

signal = sc.sc_signal[bool]("signal")
try:
    with sc.vcd_trace("setup_failure", time_unit=(1,)):
        raise AssertionError("invalid setup accepted")
except TypeError:
    pass
with sc.vcd_trace("after_failure", time_unit=(1, sc.SC_NS)) as trace:
    sc.sc_trace(trace, signal, "signal")
    sc.sc_start(1, sc.SC_NS)
    signal.write(True)
    sc.sc_start(1, sc.SC_NS)
text = Path("after_failure.vcd").read_text()
assert "$enddefinitions" in text and "#1" in text
identifier = next(line.split()[3] for line in text.splitlines() if line.startswith("$var"))
assert f"0{identifier}" in text and f"1{identifier}" in text
try:
    with sc.vcd_trace(Path("missing_directory") / "trace") as trace:
        sc.sc_trace(trace, signal, "signal")
        sc.sc_start(1, sc.SC_NS)
except Exception as error:
    assert "trace" in str(error).lower(), str(error)
else:
    raise AssertionError("invalid trace location accepted")
sc.sc_stop()
print("TRACE_FAILURE_OK")
