"""Exercise deferred arbitrary-width signal updates and retained observers."""

import gc
import sys
import weakref

from amal.eda.systemc import sc_core as sc, sc_dt


class Observer:
    """Record value-change callbacks while retaining only native registration ownership."""

    def __init__(self, signal, width, records):
        """Store the channel and output list needed by later native dispatch."""
        self.signal = signal
        self.width = width
        self.records = records

    def __call__(self):
        """Append the full signal value, simulation time and delta count."""
        value = self.signal.read()
        self.records.append(
            (
                self.width,
                len(self.records) + 1,
                int(sc.sc_time_stamp().value()),
                int(sc.sc_delta_count()),
                str(value.to_string(sc_dt.SC_DEC)),
            )
        )


def _emit_records(records):
    """Print stable native-comparable event records and the final event count."""
    for width, index, ticks, delta, value in records:
        print(f"WIDE_SIGNAL_EVENT|{width}|{index}|{ticks}|{delta}|{value}")
    print(f"WIDE_SIGNAL_RESULT|{records[0][0]}|events={len(records)}|lifetime=retained")


width = int(sys.argv[1])
assert width in (65, 137, 257), width
value_type = sc_dt.sc_biguint[width]
module_name = sc.sc_module_name(f"wide_signal_{width}")
top = sc.sc_module(module_name)
signal = sc.sc_signal[value_type](f"signal_{width}")
del module_name

records = []
callback = Observer(signal, width, records)
callback_ref = weakref.ref(callback)
top.sc_method("observe_value_change", callback)
top.sensitive(signal.default_event())
top.dont_initialize()
del callback
gc.collect()
assert callback_ref() is not None, "native method registration did not retain its callback"
assert records == [], "dont_initialize observer ran before a signal change"

high = value_type(0)
high[width - 1] = True
high_value = high.to_string(sc_dt.SC_DEC)
signal.write(high)
assert signal.read().to_string(sc_dt.SC_DEC) == "0", "signal write was visible before channel update"
sc.sc_start(sc.SC_ZERO_TIME)
assert len(records) == 1, records
assert records[-1][4] == str(high_value), records

signal.write(high)
sc.sc_start(1, sc.SC_NS)
assert len(records) == 1, "writing an unchanged wide value triggered an event"

high_and_low = value_type(high)
high_and_low[0] = True
previous = str(signal.read().to_string(sc_dt.SC_DEC))
signal.write(high_and_low)
assert signal.read().to_string(sc_dt.SC_DEC) == previous, "low-bit write was not deferred"
sc.sc_start(1, sc.SC_NS)
assert len(records) == 2, records
assert records[-1][4] == str(high_and_low.to_string(sc_dt.SC_DEC)), records

low = value_type(1)
previous = str(signal.read().to_string(sc_dt.SC_DEC))
signal.write(low)
assert signal.read().to_string(sc_dt.SC_DEC) == previous, "high-bit clear was not deferred"
sc.sc_start(1, sc.SC_NS)
assert len(records) == 3, records
assert records[-1][4] == str(low.to_string(sc_dt.SC_DEC)), records

assert [row[4] for row in records] == [str(high.to_string(sc_dt.SC_DEC)), str(high_and_low.to_string(sc_dt.SC_DEC)), "1"]
ns = int(sc.sc_time(1, sc.SC_NS).value())
assert [row[2] for row in records] == [0, ns, 2 * ns], records
assert all(records[index][3] < records[index + 1][3] for index in range(len(records) - 1)), records
assert records[0][3] >= 0, records
assert callback_ref() is not None, "registered callback was released before kernel stop"
sc.sc_stop()
_emit_records(records)
