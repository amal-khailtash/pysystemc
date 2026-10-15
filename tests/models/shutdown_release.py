"""Prove callback and module release at normal interpreter shutdown."""

import gc
import sys
from amal.eda.systemc import sc_core as sc


class Callback:
    """Own a release-record stream until the callback is destroyed."""

    def __init__(self, path):
        """Capture shutdown-safe I/O without relying on cleared module globals."""
        self.stream = open(path, "ab", buffering=0)
        self.calls = 0

    def __call__(self):
        """Count native method dispatch."""
        self.calls += 1

    def __del__(self):
        """Record actual callback destruction, including late interpreter cleanup."""
        self.stream.write(b"callback\n")
        self.stream.close()


class Model(sc.sc_module):
    """Retain bound and standalone callbacks until native host cleanup."""

    def __init__(self, name, path):
        """Build a callback cycle and a separately owned callable."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.stream = open(path, "ab", buffering=0)
        self.calls = 0
        self.sc_method("bound", self.tick)
        callback = Callback(path)
        self.sc_method("standalone", callback)

    def tick(self):
        """Count dispatch to a bound method with cyclic Python ownership."""
        self.calls += 1

    def __del__(self):
        """Record collection of the Python module owner."""
        self.stream.write(b"module\n")
        self.stream.close()


models = [Model(f"owner_{index}", sys.argv[1]) for index in range(16)]
gc.collect()
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
gc.collect()
assert all(model.calls == 1 for model in models)
assert all(model._process_callbacks[1].calls == 1 for model in models)
print("SHUTDOWN_READY")
