"""Verify Python module identity, callbacks, lifecycle and native isolation."""

import gc
from amal.eda.systemc import sc_core as sc
import cppyy

assert cppyy.gbl.sc_core.sc_module is not sc.sc_module
assert sc.sc_module is cppyy.gbl.sc_core.ScModule
records = []


class Model(sc.sc_module):
    """Record lifecycle and callback identity for one native host."""

    def __init__(self, name: str):
        """Register independent callbacks and reject invalid registrations."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.sc_method("tick", self.tick)
        try:
            self.sc_method("invalid", None)
        except TypeError:
            pass
        else:
            raise AssertionError("invalid callback accepted")
        assert len(self._process_callbacks) == 1

    def tick(self):
        """Record the native host name after garbage collection."""
        records.append((self.name(), "tick"))

    def before_end_of_elaboration(self):
        """Record elaboration entry."""
        records.append((self.name(), "before"))

    def end_of_elaboration(self):
        """Record elaboration completion."""
        records.append((self.name(), "elaborated"))

    def start_of_simulation(self):
        """Record simulation entry."""
        records.append((self.name(), "start"))

    def end_of_simulation(self):
        """Record simulation completion."""
        records.append((self.name(), "end"))


models = [Model("first"), Model("second")]
gc.collect()
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
for model in models:
    assert [event for name, event in records if name == model.name()] == [
        "before",
        "elaborated",
        "start",
        "tick",
        "end",
    ]
print("MODULE_ISOLATION_OK")
