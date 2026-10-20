"""Verify structural adapters preserve native identity through inherited interfaces."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy


class Signal:
    """Own a wide value and its stable native notification event."""

    def __init__(self):
        """Allocate storage retained for the native interface lifetime."""
        self.value = sc_dt.sc_biguint[137](17)
        self.changed = sc.sc_event()

    def read(self):
        """Borrow the current wide value."""
        return self.value

    def get_data_ref(self):
        """Borrow the same stable tracing storage."""
        return self.value

    def event(self):
        """Return notification status in the current delta cycle."""
        return self.changed.triggered()

    def value_changed_event(self):
        """Borrow the shared value-change event."""
        return self.changed

    def write(self, value):
        """Copy a new value and schedule a delta notification."""
        self.value.__assign__(value)
        self.changed.notify(sc.SC_ZERO_TIME)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
owner = Signal()
value_type = sc_dt.sc_biguint[137]
writer = sc.sc_port[sc.sc_signal_inout_if[value_type]]("writer")
writer(owner)
reader = sc.sc_port[sc.sc_signal_in_if[value_type]]("reader")
reader(owner)
write_only = sc.sc_port[sc.sc_signal_write_if[value_type]]("write_only")
write_only(owner)
repeat = sc.sc_port[sc.sc_signal_in_if[value_type]]("repeat")
repeat(owner)
exported = sc.sc_export[sc.sc_signal_in_if[value_type]]("exported")
exported(owner)
export_chain = sc.sc_export[sc.sc_signal_in_if[value_type]]("export_chain")
export_chain(exported)
export_reader = sc.sc_port[sc.sc_signal_in_if[value_type]]("export_reader")
export_reader(export_chain)


class Queue:
    """Provide combined FIFO and TLM get/peek callbacks for inherited views."""

    def __init__(self):
        """Retain integer samples and a native readiness event."""
        self.values = [41, 43]
        self.changed = sc.sc_event()

    def read(self):
        """Consume a sample through either blocking read overload."""
        return self.values.pop(0)

    def nb_read(self):
        """Return success and a sample, preserving failed native outputs."""
        return (True, self.read()) if self.values else (False, 0)

    def num_available(self):
        """Return the number of queued samples."""
        return len(self.values)

    def data_written_event(self):
        """Borrow the native reader notification event."""
        return self.changed

    def get(self):
        """Consume one sample through the TLM blocking interface."""
        return self.read()

    def peek(self):
        """Borrow the first sample by value without consuming it."""
        return self.values[0]

    def nb_get(self):
        """Return a nonblocking TLM sample pair."""
        return self.nb_read()

    def nb_peek(self):
        """Return a nonblocking TLM peek pair."""
        return (True, self.peek()) if self.values else (False, 0)

    def nb_can_get(self):
        """Report TLM sample availability."""
        return bool(self.values)

    def nb_can_peek(self):
        """Report TLM peek availability."""
        return bool(self.values)


reverse_signal_owner = Signal()
reverse_signal_ports = []
for contract in (sc.sc_signal_write_if, sc.sc_signal_in_if, sc.sc_signal_inout_if):
    port = sc.sc_port[contract[value_type]]()
    port(reverse_signal_owner)
    reverse_signal_ports.append(port)
reverse_fifo_owner = Queue()
reverse_fifo_ports = []
for contract in (sc.sc_fifo_nonblocking_in_if, sc.sc_fifo_blocking_in_if, sc.sc_fifo_in_if):
    port = sc.sc_port[contract[int]]()
    port(reverse_fifo_owner)
    reverse_fifo_ports.append(port)

fifo_owner = Queue()
fifo_ports = []
for contract in (sc.sc_fifo_in_if, sc.sc_fifo_blocking_in_if, sc.sc_fifo_nonblocking_in_if):
    port = sc.sc_port[contract[int]]()
    port(fifo_owner)
    fifo_ports.append(port)
reverse_tlm_owner = Queue()
reverse_tlm_ports = []
for contract in (tlm.tlm_nonblocking_peek_if, tlm.tlm_blocking_get_if, tlm.tlm_get_peek_if):
    port = sc.sc_port[contract[int]]()
    port(reverse_tlm_owner)
    reverse_tlm_ports.append(port)
permuted_tlm_groups = []
for contracts in (
    (tlm.tlm_blocking_peek_if, tlm.tlm_nonblocking_get_if, tlm.tlm_get_peek_if),
    (tlm.tlm_nonblocking_get_if, tlm.tlm_nonblocking_peek_if, tlm.tlm_get_peek_if),
):
    queue = Queue()
    ports = []
    for contract in contracts:
        port = sc.sc_port[contract[int]]()
        port(queue)
        ports.append(port)
    permuted_tlm_groups.append((queue, ports))
tlm_owner = Queue()
tlm_ports = []
for contract in (
    tlm.tlm_get_peek_if,
    tlm.tlm_blocking_get_if,
    tlm.tlm_nonblocking_get_if,
    tlm.tlm_blocking_peek_if,
    tlm.tlm_nonblocking_peek_if,
):
    port = sc.sc_port[contract[int]]()
    port(tlm_owner)
    tlm_ports.append(port)
# Multiword native types must compile and preserve signed/unsigned endpoints.
scalar_cases = []
for scalar, values in (("long long", [-(2**63), 2**63 - 1]), ("unsigned long long", [0, 2**64 - 1])):
    scalar_owner = Queue()
    scalar_owner.values = values.copy()
    scalar_port = sc.sc_port[sc.sc_fifo_in_if[scalar]]()
    scalar_port(scalar_owner)
    scalar_cases.append((scalar_owner, scalar_port, values))
other_type = scalar_cases[0][1]

del name

cppyy.cppdef("""
/** Return the shared virtual interface address without taking ownership.
 * The caller retains the channel; no callback or GIL transition occurs.
 */
std::uintptr_t structural_interface_address(sc_core::sc_interface* value) {
    return reinterpret_cast<std::uintptr_t>(value);
}
""")
interfaces = [port.get_interface() for port in (writer, reader, write_only, repeat, exported, export_chain)]
addresses = [cppyy.gbl.structural_interface_address(interface) for interface in interfaces]
assert len(set(addresses)) == 1, addresses
signal_address = addresses[0]
assert all(cppyy.addressof(interface.default_event()) == cppyy.addressof(owner.changed) for interface in interfaces)
for group in (fifo_ports, tlm_ports, reverse_signal_ports, reverse_fifo_ports, reverse_tlm_ports):
    addresses = [cppyy.gbl.structural_interface_address(port.get_interface()) for port in group]
    assert len(set(addresses)) == 1, addresses
for queue, ports in permuted_tlm_groups:
    interfaces = [port.get_interface() for port in ports]
    assert len({cppyy.gbl.structural_interface_address(interface) for interface in interfaces}) == 1
    combined = interfaces[-1]
    for interface in interfaces:
        for method in ("ok_to_get", "ok_to_peek"):
            if hasattr(interface, method):
                assert cppyy.addressof(getattr(interface, method)()) == cppyy.addressof(getattr(combined, method)())
    assert combined.peek() == 41
    assert combined.get() == 41
    assert combined.peek() == 43
assert cppyy.gbl.structural_interface_address(other_type.get_interface()) != cppyy.gbl.structural_interface_address(
    fifo_ports[0].get_interface()
)
reverse_signal_ports[0].get_interface().write(value_type(31))
assert reverse_signal_ports[1].get_interface().read().to_uint64() == 31
assert reverse_fifo_ports[1].get_interface().read() == 41
assert reverse_fifo_ports[2].get_interface().num_available() == 1
assert fifo_ports[1].get_interface().read() == 41
assert fifo_ports[0].get_interface().num_available() == 1
assert reverse_tlm_ports[1].get_interface().get() == 41
assert reverse_tlm_ports[2].get_interface().peek() == 43
assert tlm_ports[3].get_interface().peek() == 41
assert tlm_ports[1].get_interface().get() == 41
assert tlm_ports[0].get_interface().peek() == 43
print("IDENTITY_REFERENCE 41 1 41 43", sc.sc_time_stamp())

for scalar_owner, scalar_port, values in scalar_cases:
    assert [scalar_port.get_interface().read(), scalar_port.get_interface().read()] == values
ready_seen = []
permuted_ready_seen = []
for index, (queue, ports) in enumerate(permuted_tlm_groups):
    combined = ports[-1].get_interface()
    for method in ("ok_to_get", "ok_to_peek"):

        def record_ready(index=index, method=method):
            """Record readiness through each borrowed partial-family event."""
            permuted_ready_seen.append((index, method, str(sc.sc_time_stamp())))

        top.sc_method(f"permuted_ready_{index}_{method}", record_ready)
        top.sensitive(getattr(combined, method)())
        top.dont_initialize()
    combined.notify_ready(sc.sc_time(1, sc.SC_NS))
combined_tlm = reverse_tlm_ports[2].get_interface()
get_event = reverse_tlm_ports[1].get_interface().ok_to_get()
peek_event = reverse_tlm_ports[0].get_interface().ok_to_peek()
assert cppyy.addressof(get_event) == cppyy.addressof(combined_tlm.ok_to_get())
assert cppyy.addressof(peek_event) == cppyy.addressof(combined_tlm.ok_to_peek())
assert cppyy.addressof(get_event) != cppyy.addressof(peek_event)
top.sc_method("observe_ready", lambda: ready_seen.append(str(sc.sc_time_stamp())))
top.sensitive(peek_event)
top.dont_initialize()
combined_tlm.notify_ready(sc.sc_time(1, sc.SC_NS))
seen = []
top.sc_method("observe", lambda: seen.append((reader.get_interface().read().to_uint64(), str(sc.sc_time_stamp()))))
top.sensitive(reader.get_interface().value_changed_event())
top.dont_initialize()
write_only.get_interface().write(value_type(29))
sc.sc_start(2, sc.SC_NS)
assert seen == [(29, "0 s")], seen
assert ready_seen == ["1 ns"], ready_seen
assert sorted(permuted_ready_seen) == [
    (index, method, "1 ns") for index in range(len(permuted_tlm_groups)) for method in ("ok_to_get", "ok_to_peek")
], permuted_ready_seen
print("IDENTITY_READY", ready_seen[0])
assert writer.get_interface().read().to_uint64() == 29
assert export_reader.get_interface().read().to_uint64() == 29
assert cppyy.gbl.structural_interface_address(export_reader.get_interface()) == signal_address
sc.sc_stop()
print("STRUCTURAL_BASE_IDENTITY_OK")
