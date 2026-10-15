"""Verify structural master/slave families with distinct native request/response types."""

from amal.eda.systemc import sc_core as sc, tlm
import cppyy
import ctypes


def number(value):
    """Read a native integral value without relying on Python integer conversion."""
    return value.to_uint64() if hasattr(value, "to_uint64") else value.value if hasattr(value, "value") else int(value)


class Endpoint:
    """Retain values and record ordinary blocking/nonblocking callback execution."""

    def __init__(self):
        """Initialize independent put and get state."""
        self.sent = []
        self.values = [23, 41]

    def put(self, value):
        """Store a copied value after one nanosecond of native suspension."""
        sc.wait(1, sc.SC_NS)
        self.sent.append(number(value))

    def get(self):
        """Consume a queued sample."""
        return self.values.pop(0)

    def peek(self):
        """Return the next sample without consuming it."""
        return self.values[0]

    def nb_put(self, value):
        """Copy a value without waiting."""
        self.sent.append(number(value))
        return True

    def nb_get(self):
        """Consume only on success, returning a native-output tuple."""
        return (True, self.values.pop(0)) if self.values else (False, 0)

    def nb_peek(self):
        """Preserve queue state and report empty failure."""
        return (True, self.values[0]) if self.values else (False, 0)

    def nb_can_put(self):
        """Report unbounded capacity."""
        return True

    def nb_can_get(self):
        """Report sample availability."""
        return bool(self.values)

    def nb_can_peek(self):
        """Report nonconsuming sample availability."""
        return bool(self.values)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
ports = []
for mode in ("", "blocking_", "nonblocking_"):
    for direction in ("master", "slave"):
        interface = getattr(tlm, f"tlm_{mode}{direction}_if")[int, cppyy.gbl.sc_dt.sc_biguint[73]]
        port = sc.sc_port[interface](f"{mode}{direction}")
        owner = Endpoint()
        port(owner)
        ports.append((mode, direction, port, owner))
del name
records = []


def exercise():
    """Check both native overloads, tuple assignment, failure retention and timing."""
    for mode, direction, port, owner in ports:
        interface = port.get_interface()
        output_type = cppyy.gbl.sc_dt.sc_biguint[73] if direction == "master" else ctypes.c_int
        if mode != "nonblocking_":
            interface.put(17)
            assert number(interface.peek()) == 23
            value = output_type(0)
            interface.peek(value)
            assert number(value) == 23
            assert number(interface.get()) == 23
            interface.get(value)
            assert number(value) == 41
        if mode != "blocking_":
            owner.values = [23, 41]
            assert interface.nb_can_put() and interface.nb_can_get() and interface.nb_can_peek()
            assert interface.nb_put(19)
            value = output_type(0)
            assert interface.nb_peek(value) and number(value) == 23
            assert interface.nb_get(value) and number(value) == 23
            assert interface.nb_get(value) and number(value) == 41
            assert not interface.nb_get(value) and number(value) == 41
            assert not interface.nb_peek(value) and number(value) == 41
            assert not interface.nb_can_get() and not interface.nb_can_peek()
            assert interface.ok_to_put() != interface.ok_to_get()
            assert interface.ok_to_get() != interface.ok_to_peek()
        assert owner.sent == ([17, 19] if not mode else [17] if mode == "blocking_" else [19])
        records.append((mode, direction))


handle = sc.sc_spawn(exercise, "exercise")
sc.sc_start(5, sc.SC_NS)
assert handle.terminated() and len(records) == 6, records
assert sc.sc_time_stamp() == sc.sc_time(5, sc.SC_NS)
sc.sc_stop()
print("STRUCTURAL_TLM_MASTER_SLAVE_OK")
