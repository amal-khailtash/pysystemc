"""Validate queued notifications and native resolved logic signals."""

from amal.eda.systemc import sc_core as sc, sc_dt as dt


class Model(sc.sc_module):
    """Own queued events and resolved channels throughout simulation."""

    def __init__(self, name):
        """Register observers and independent resolved signal writers."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.queue = sc.sc_event_queue("queue")
        self.resolved = sc.sc_signal_resolved("resolved")
        self.seen = []
        self.sc_method("observe", self.observe)
        self.sensitive(self.queue.default_event())
        self.dont_initialize()
        self.sc_thread("zero", self.zero)
        self.sc_thread("one", self.one)

    def observe(self):
        """Record each queue notification at its native simulation timestamp."""
        self.seen.append(str(sc.sc_time_stamp()))

    def zero(self):
        """Drive a persistent zero from the first native writer process."""
        self.resolved.write(dt.sc_logic("0"))

    def one(self):
        """Drive a persistent one from a second independent native writer."""
        self.resolved.write(dt.sc_logic("1"))


model = Model("extensions")
model.queue.notify(1, sc.SC_NS)
model.queue.notify(2, sc.SC_NS)
model.queue.notify(2, sc.SC_NS)
sc.sc_start(3, sc.SC_NS)
assert model.seen == ["1 ns", "2 ns", "2 ns"], model.seen
assert model.resolved.read().to_char() == "X"
model.queue.notify(1, sc.SC_NS)
model.queue.cancel_all()
sc.sc_start(2, sc.SC_NS)
assert len(model.seen) == 3
sc.sc_stop()
print("CHANNEL_EXTENSIONS_OK")
