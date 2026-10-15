"""Validate stable reference returns from a Python native signal interface."""

from amal.eda.systemc import sc_core as sc


class Signal(sc.signal_channel[int]):
    """Own stable native value/event references behind a Python channel object."""

    def __init__(self, name):
        """Initialize retained native signal storage."""
        super().__init__(name)


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
signal = Signal("signal")
port = sc.sc_in[int]("input")
port.bind(signal)
seen = []
model.sc_method("observe", lambda: seen.append(port.read()))
model.sensitive(port)
model.dont_initialize()
del module_name
signal.write(17)
sc.sc_start(1, sc.SC_NS)
assert port.read() == 17
assert seen == [17], seen
signal.write(23)
sc.sc_start(1, sc.SC_NS)
assert port.read() == 23
assert seen == [17, 23], seen
sc.sc_stop()
print("NATIVE_SIGNAL_INTERFACE_OK")
