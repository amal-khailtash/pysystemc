"""Verify direct finder retention until static sensitivity has been resolved."""

import gc
import weakref

from amal.eda.systemc import sc_core as sc


class FinderModel(sc.sc_module):
    """Retain a direct finder until the kernel resolves port sensitivity."""

    def __init__(self):
        """Create live signal/port owners and register timed process callbacks."""
        name = sc.sc_module_name("finder_lifetime")
        super().__init__(name)
        self.signal = sc.sc_signal[bool]("signal")
        self.port = sc.sc_in[bool]("port")
        self.port.bind(self.signal)
        interface = sc.sc_signal_in_if[bool]
        self.finder = sc.sc_event_finder_t[interface](self.port, interface.value_changed_event)
        self.transitions = []
        self.sc_method("observe", self.observe)
        self.sensitive(self.finder)
        self.dont_initialize()
        self.sc_thread("drive", self.drive)
        del name

    def observe(self):
        """Record signal transitions delivered by resolved static sensitivity."""
        self.transitions.append((sc.sc_time_stamp().value(), self.port.read()))

    def drive(self):
        """Change the retained signal at two distinct simulation times."""
        sc.wait(1, sc.SC_NS)
        self.signal.write(True)
        sc.wait(1, sc.SC_NS)
        self.signal.write(False)


model = FinderModel()
finder_ref = weakref.ref(model.finder)
gc.collect()
assert finder_ref() is not None
sc.sc_start(0, sc.SC_NS)
assert sc.sc_get_status() == sc.SC_PAUSED
assert model.transitions == []
# Port binding has resolved static sensitivity to the signal's native event.
del model.finder
gc.collect()
assert finder_ref() is None
sc.sc_start(3, sc.SC_NS)
assert model.transitions == [(1000, True), (2000, False)], model.transitions
sc.sc_stop()
print("FINDER_ELABORATION_LIFETIME_OK retained=1 released=1 transitions=1000,2000")
