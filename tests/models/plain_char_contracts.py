"""Check plain char signal/FIFO endpoints, native outputs and rejected samples."""

from amal.eda.systemc import sc_core as sc
import cppyy


class Owner:
    """Supply an integer sample and a retained native readiness event."""

    def __init__(self):
        """Initialize sample and event before binding."""
        self.value = 7
        self.changed = sc.sc_event()

    def read(self):
        """Return the configured plain char integer sample."""
        return self.value

    def event(self):
        """Report native event state."""
        return self.changed.triggered()

    def value_changed_event(self):
        """Borrow the signal event for the adapter lifetime."""
        return self.changed

    def nb_read(self):
        """Return a successful FIFO sample."""
        return True, self.value

    def num_available(self):
        """Report one available sample."""
        return 1

    def data_written_event(self):
        """Borrow the FIFO event for the adapter lifetime."""
        return self.changed


cppyy.cppdef("""
/** Inspect borrowed plain char interfaces with the callback GIL held.
 * Outputs are local; exceptions propagate without retaining storage.
 */
namespace plain_char_check {
int signal(sc_core::sc_signal_in_if<char>& channel) { return channel.read(); }
int reference(sc_core::sc_signal_in_if<char>& channel) { return channel.get_data_ref(); }
int blocking(sc_core::sc_fifo_in_if<char>& channel) { char value = 7; channel.read(value); return value; }
int nonblocking(sc_core::sc_fifo_in_if<char>& channel) {
  char value = 7; if (!channel.nb_read(value)) throw std::runtime_error("missing sample"); return value;
}
}
""")
name = sc.sc_module_name("top")
top = sc.sc_module(name)
signal_owner, fifo_owner = Owner(), Owner()
signal_port = sc.sc_port[sc.sc_signal_in_if["char"]]()
fifo_port = sc.sc_port[sc.sc_fifo_in_if["char"]]()
signal_port(signal_owner)
fifo_port(fifo_owner)
del name
signal, fifo = signal_port.get_interface(), fifo_port.get_interface()
limits = cppyy.gbl.std.numeric_limits["char"]
low, high = int(limits.is_signed), cppyy.sizeof("char") * 8
low, high = (-(2 ** (high - 1)), 2 ** (high - 1) - 1) if low else (0, 2**high - 1)
for value in (low, high, 0, 7):
    signal_owner.value = fifo_owner.value = value
    results = (
        cppyy.gbl.plain_char_check.signal(signal),
        cppyy.gbl.plain_char_check.reference(signal),
        cppyy.gbl.plain_char_check.blocking(fifo),
        cppyy.gbl.plain_char_check.nonblocking(fifo),
    )
    assert results == (value,) * 4, results
    print("PLAIN_CHAR", value, *results)
for invalid in (True, 1.5, "a", object(), low - 1, high + 1):
    signal_owner.value = invalid
    for method in (signal.read, signal.get_data_ref):
        try:
            method()
        except TypeError:
            pass
        else:
            raise AssertionError("invalid plain char signal accepted")
        sampled = signal.sampled
        numeric = ord(sampled) if isinstance(sampled, str) else sampled
        assert numeric == 7, numeric
    signal_owner.value = 7
    assert cppyy.gbl.plain_char_check.signal(signal) == 7
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
print("PLAIN_CHAR_CONTRACTS_OK")
