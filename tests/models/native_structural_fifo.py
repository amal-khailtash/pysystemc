"""Validate structural FIFO blocking/output overloads and shared borrowed events."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy


class Owner:
    """Implement one bounded queue for native input/output adapters."""

    def __init__(self):
        """Create shared queue and notification events."""
        self.queue = []
        self.written = sc.sc_event("written")
        self.read_event = sc.sc_event("read_event")

    def read(self):
        """Wait with guarded kernel semantics, then consume a copied value."""
        while not self.queue:
            sc.wait(self.written)
        value = self.queue.pop(0)
        self.read_event.notify(sc.SC_ZERO_TIME)
        return value

    def nb_read(self):
        """Preserve native output when no value is available."""
        return (True, self.read()) if self.queue else (False, sc_dt.sc_biguint[137](0))

    def write(self, value):
        """Copy borrowed input before notifying readers."""
        self.queue.append(sc_dt.sc_biguint[137](value))
        self.written.notify(sc.SC_ZERO_TIME)

    def nb_write(self, value):
        """Accept a value into the bounded queue."""
        if not self.num_free():
            return False
        self.write(value)
        return True

    def num_available(self):
        """Count queued values."""
        return len(self.queue)

    def num_free(self):
        """Count free slots."""
        return 2 - len(self.queue)

    def data_written_event(self):
        """Borrow the reader wakeup event."""
        return self.written

    def data_read_event(self):
        """Borrow the writer wakeup event."""
        return self.read_event


name = sc.sc_module_name("top")
top = sc.sc_module(name)
owner = Owner()
reader = sc.sc_port[sc.sc_fifo_in_if[sc_dt.sc_biguint[137]]]("reader")
writer = sc.sc_port[sc.sc_fifo_out_if[sc_dt.sc_biguint[137]]]("writer")
reader(owner)
writer(owner)
del name
incoming = reader.get_interface()
outgoing = writer.get_interface()
assert cppyy.addressof(incoming.data_written_event()) == cppyy.addressof(owner.written)
assert cppyy.addressof(outgoing.data_read_event()) == cppyy.addressof(owner.read_event)
value = sc_dt.sc_biguint[137](73)
assert not incoming.nb_read(value) and value.to_uint() == 73
seen = []


def consume():
    """Exercise both blocking read overloads and actual scheduling."""
    incoming.read(value)
    assert value[136] and value.to_uint() == 17
    seen.append(sc.sc_time_stamp().to_double())
    second = incoming.read()
    assert second.to_uint() == 29
    seen.append(sc.sc_time_stamp().to_double())


def produce():
    """Deliver two values at distinct native times."""
    sc.wait(2, sc.SC_NS)
    payload = sc_dt.sc_biguint[137](17)
    payload[136] = True
    outgoing.write(payload)
    sc.wait(1, sc.SC_NS)
    assert outgoing.nb_write(sc_dt.sc_biguint[137](29))


top.sc_thread("consume", consume)
top.sc_thread("produce", produce)
sc.sc_start(4, sc.SC_NS)
assert seen == [2000, 3000], seen
assert incoming.num_available() == 0 and outgoing.num_free() == 2
sc.sc_stop()
print("FIFO_REFERENCE 2 3 4 ns")
print("NATIVE_STRUCTURAL_FIFO_OK")
