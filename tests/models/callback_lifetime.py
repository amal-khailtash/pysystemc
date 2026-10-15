"""Verify callback retention through garbage collection and kernel stop."""

import gc
import weakref
from amal.eda.systemc import sc_core as sc


class Callback:
    """Record invocations while exposing callback reference lifetime."""

    def __init__(self, seen):
        """Retain the externally owned observation list."""
        self.seen = seen

    def __call__(self):
        """Record one native method dispatch."""
        self.seen.append(str(sc.sc_time_stamp()))


seen = []
module_name = sc.sc_module_name("lifetime")
model = sc.sc_module(module_name)
del module_name
callback = Callback(seen)
reference = weakref.ref(callback)
model.sc_method("tick", callback)
del callback
gc.collect()
assert reference() is not None
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
gc.collect()
assert seen == ["0 s"]
assert reference() is not None, "stop released a callback still owned by a native host"
print("CALLBACK_LIFETIME_OK")
