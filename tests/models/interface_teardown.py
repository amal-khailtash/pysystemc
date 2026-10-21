"""Verify native borrower rejection and explicit adapter destruction after stop."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy
import gc
import weakref
from pathlib import Path


class Owner:
    """Provide an analysis callback retained by its structural adapter."""

    def write(self, value):
        """Accept copied native input without retaining it."""
        assert value == 17


class ChannelOwner:
    """Supply signal, FIFO and TLM callbacks with retained wide storage/events."""

    def __init__(self, wide=False):
        """Allocate storage that callbacks may return by native reference."""
        self.value = sc_dt.sc_biguint[137](17) if wide else 17
        if wide:
            self.value[136] = True
        self.changed = sc.sc_event()

    def read(self):
        """Return retained scalar or wide storage."""
        return self.value

    def get_data_ref(self):
        """Borrow the same native storage for signal reference access."""
        return self.value

    def event(self):
        """Report native event state."""
        return self.changed.triggered()

    def value_changed_event(self):
        """Borrow the signal event."""
        return self.changed

    def nb_read(self):
        """Return the successful FIFO sample."""
        return True, self.value

    def num_available(self):
        """Report one FIFO sample."""
        return 1

    def data_written_event(self):
        """Borrow the FIFO event."""
        return self.changed

    def get(self):
        """Return the TLM sample."""
        return self.value

    def peek(self):
        """Return the TLM sample without mutation."""
        return self.value

    def nb_get(self):
        """Return a successful TLM get."""
        return True, self.value

    def nb_peek(self):
        """Return a successful TLM peek."""
        return True, self.value

    def nb_can_get(self):
        """Report TLM get readiness."""
        return True

    def nb_can_peek(self):
        """Report TLM peek readiness."""
        return True


name = sc.sc_module_name("top")
top = sc.sc_module(name)
groups = []
owner_refs = []
result_refs = []
for datatype in ("int", sc_dt.sc_biguint[137]):
    for family in (sc.sc_signal_in_if, sc.sc_fifo_in_if, tlm.tlm_get_peek_if):
        channel_owner = ChannelOwner(datatype != "int")
        channel_port = sc.sc_port[family[datatype]]()
        channel_port(channel_owner)
        interface = channel_port.get_interface()
        sample = interface.peek() if family is tlm.tlm_get_peek_if else interface.read()
        if datatype != "int":
            assert bool(sample[136]) and sample.to_uint64() == 17
            result_refs.append(weakref.ref(channel_owner.value))
        owner_refs.append(weakref.ref(channel_owner))
        groups.append((family, channel_port, interface.name()))
del channel_owner, channel_port, interface, sample
owner = Owner()
reference = weakref.ref(owner)
port = sc.sc_port[tlm.tlm_analysis_if[int]]()
port(owner)
export = sc.sc_export[tlm.tlm_analysis_if[int]]()
export(owner)
cppyy.cppdef("""
/** Native borrower constructed without Python bind interception; caller owns deletion. */
struct TeardownBorrower : sc_core::sc_module {
  sc_core::sc_port<tlm::tlm_analysis_if<int>> port;
  explicit TeardownBorrower(sc_core::sc_module_name name) : sc_module(name), port("port") {}
};
""")
native_name = sc.sc_module_name("native_borrower")
native_borrower = cppyy.gbl.TeardownBorrower(native_name)
native_borrower.port.bind(port.get_interface())
del native_name
native = port.get_interface()
adapter_name = native.name()
native.write(17)
del name, owner
try:
    sc.teardown_interfaces()
except RuntimeError:
    pass
else:
    raise AssertionError("pre-stop deletion accepted")
trace = sc.sc_create_vcd_trace_file("teardown")
trace.set_time_unit(1, sc.SC_NS)
wide_signal = groups[3][1].get_interface()
sc.sc_trace(trace, wide_signal, "wide")
sc.sc_start(1, sc.SC_NS)
wide_owner = owner_refs[3]()
wide_owner.value[0] = False
wide_owner.value[1] = True
sc.sc_start(1, sc.SC_NS)
sc.sc_close_vcd_trace_file(trace)
vcd = Path("teardown.vcd").read_text()
assert "$var wire   137" in vcd or any("137" in line and "wide" in line for line in vcd.splitlines())
transitions = [line for line in vcd.splitlines() if line.startswith("b")]
assert len(set(transitions)) >= 2, vcd
del wide_owner, wide_signal
sc.sc_stop()
try:
    sc.teardown_interfaces()
except RuntimeError:
    pass
else:
    raise AssertionError("borrowed adapter deletion accepted")
assert reference() is not None
native.write(17)
port.__destruct__()
for borrower in (export, native_borrower):
    try:
        sc.teardown_interfaces()
    except RuntimeError:
        pass
    else:
        raise AssertionError("remaining native/export borrower ignored")
    borrower.__destruct__()
for family, channel_port, channel_name in groups:
    interface = channel_port.get_interface()
    if family is tlm.tlm_get_peek_if:
        assert interface.nb_can_get() and interface.nb_can_peek()
        assert cppyy.addressof(interface.ok_to_get()) != cppyy.addressof(interface.ok_to_peek())
        interface.peek()
    else:
        interface.read()
    channel_port.__destruct__()
assert sc.teardown_interfaces() == 7
for family, channel_port, channel_name in groups:
    assert not cppyy.gbl.sc_core.sc_find_object(channel_name)
assert not cppyy.gbl.sc_core.sc_find_object(adapter_name)
gc.collect()
assert reference() is None
assert all(ref() is None for ref in owner_refs + result_refs)
assert sc.teardown_interfaces() == 0
print("INTERFACE_TEARDOWN_OK 7 2 ns")
