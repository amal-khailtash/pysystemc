"""Verify static method sensitivity and initialization under garbage collection."""

import gc
from amal.eda.systemc import sc_core as sc


class Model(sc.sc_module):
    """Observe an interface, port, edge finder and standalone event."""

    def __init__(self, name):
        """Retain native sensitivity objects throughout simulation."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.signal = sc.sc_signal[bool]("signal")
        self.port = sc.sc_in[bool]("port")
        self.port.bind(self.signal)
        self.event = sc.sc_event("event")
        self.edge = self.port.pos()
        self.records = []
        for kind, source in [
            ("interface", self.signal),
            ("port", self.port),
            ("edge", self.edge),
            ("event", self.event),
        ]:
            self.sc_method(kind, lambda kind=kind: self.record(kind))
            self.sensitive(source)
            self.dont_initialize()
        self.sc_method("initial", lambda: self.record("initial"))

    def record(self, kind):
        """Record identity, timestamp and channel value on each invocation."""
        self.records.append((kind, str(sc.sc_time_stamp()), self.signal.read()))


model = Model("sensitivity")
gc.collect()
sc.sc_start(1, sc.SC_NS)
assert model.records == [("initial", "0 s", False)], model.records
model.signal.write(True)
model.event.notify(sc.sc_time(1, sc.SC_NS))
gc.collect()
sc.sc_start(2, sc.SC_NS)
assert sorted(model.records[1:]) == sorted(
    [
        ("interface", "1 ns", True),
        ("port", "1 ns", True),
        ("edge", "1 ns", True),
        ("event", "2 ns", True),
    ]
), model.records
model.signal.write(False)
sc.sc_start(1, sc.SC_NS)
assert sorted(model.records[-2:]) == [("interface", "3 ns", False), ("port", "3 ns", False)], model.records
sc.sc_stop()
print("METHOD_SENSITIVITY_OK")
