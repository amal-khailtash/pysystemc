"""Verify retained callback copies for nondefault, nonassignable native values."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_core as sc, tlm
import cppyy

cppyy.include(str(Path(__file__).parents[1] / "native/sc_core_copy_only_values.h"))
transaction = cppyy.gbl.copy_only.transaction


class Sink:
    """Retain independent copies received through native write and put views."""

    def __init__(self):
        """Own callback samples and record the borrowed delay synchronously."""
        self.samples = []
        self.delay = 0

    def write(self, value, delay=None):
        """Retain a copied input and snapshot an optional borrowed delay."""
        self.samples.append(value)
        self.delay = int(delay.value()) if delay is not None else 0

    def put(self, value):
        """Retain a copied blocking put input."""
        self.samples.append(value)

    def nb_put(self, value):
        """Retain a copied nonblocking put input and accept it."""
        self.samples.append(value)
        return True

    def nb_can_put(self, tag=None):
        """Report available capacity without retaining a native tag."""
        return True

    def get_writer_policy(self):
        """Return the native one-writer policy without enforcing a second policy."""
        return sc.SC_ONE_WRITER


name = sc.sc_module_name("copy_only_values")
model = sc.sc_module(name)
sink = Sink()
ports = []
forms = (
    ("analysis", tlm.tlm_analysis_if, "write", False),
    ("write", tlm.tlm_write_if, "write", False),
    ("delayed_analysis", tlm.tlm_delayed_analysis_if, "write", True),
    ("delayed_write", tlm.tlm_delayed_write_if, "write", True),
    ("blocking_put", tlm.tlm_blocking_put_if, "put", False),
    ("nonblocking_put", tlm.tlm_nonblocking_put_if, "nb_put", False),
    ("signal_write", sc.sc_signal_write_if, "write", False),
    ("fifo_write", sc.sc_fifo_blocking_out_if, "write", False),
)

for index, (label, interface, method, delayed) in enumerate(forms):
    port = sc.sc_port[interface[transaction]](label)
    port(sink)
    ports.append(port)
    value = transaction(73 + index)
    sink.delay = 0
    args = (value, sc.sc_time(2, sc.SC_NS)) if delayed else (value,)
    accepted = getattr(port.get_interface(), method)(*args)
    assert accepted is (True if method == "nb_put" else None)
    assert sink.samples[-1].__python_owns__
    assert cppyy.addressof(sink.samples[-1]) != cppyy.addressof(value)
    sink.samples[-1].value += 100
    assert value.value == 73 + index
    del args
    value.__destruct__()
    del value
    gc.collect()
    assert sink.samples[-1].value == 173 + index
    assert transaction.live == len(sink.samples)
    print("COPY_ONLY_VALUE", label, sink.samples[-1].value, sink.delay)

# Native copy-constructor failure precedes the Python callback and preserves
# caller storage and all previously retained callback values.
value = transaction(999)
transaction.reject_copy = True
before = len(sink.samples)
try:
    ports[0].get_interface().write(value)
except cppyy.gbl.std.runtime_error as error:
    assert "copy-only transaction rejected" in str(error)
else:
    raise AssertionError("native copy failure was lost")
transaction.reject_copy = False
assert len(sink.samples) == before and value.value == 999
value.__destruct__()
del value
sink.samples.clear()
gc.collect()
assert transaction.live == 0
print("COPY_ONLY_FAILURE 1 999 0")
del name
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_stop()
assert sc.release_interface_callbacks() == 5
print("COPY_ONLY_CALLBACK_RELEASE_OK")
