"""Check inherited channel identity across native type specializations and bind orders."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy


def value_of(value):
    """Convert native integer datatypes to Python integers for readable records."""
    return value.to_uint64() if hasattr(value, "to_uint64") else value


def make_sample(value_type, expected):
    """Build a typed sample without narrowing arbitrary-width high bits."""
    if value_type is wide_type:
        value = value_type(expected & ((1 << 64) - 1))
        value[136] = bool(expected & (1 << 136))
        return value
    return value_type(expected) if hasattr(value_type, "__cpp_name__") else expected


def assert_sample(value, expected):
    """Check a scalar or arbitrary-width sample, including its highest bit."""
    if hasattr(value, "to_uint64"):
        assert value.to_uint64() == expected & ((1 << 64) - 1), (value.to_uint64(), expected)
        assert bool(value[136]) == bool(expected & (1 << 136)), (value, expected)
    else:
        assert value == expected


class SignalOwner:
    """Supply a typed signal value and one stable change event."""

    def __init__(self, value_type, initial):
        """Retain typed storage and an event borrowed by every signal view."""
        self.value_type = value_type
        self.value = value_type(initial) if hasattr(value_type, "__cpp_name__") else initial
        self.changed = sc.sc_event()
        self.writes = 0

    def read(self):
        """Return the current sample for all input interface views."""
        return self.value

    def get_data_ref(self):
        """Return the same stable sample storage for wide signal references."""
        return self.value

    def event(self):
        """Report whether the change event fired in this delta cycle."""
        return self.changed.triggered()

    def value_changed_event(self):
        """Borrow the one native event shared by all inherited views."""
        return self.changed

    def write(self, value):
        """Store a copied sample and notify all input views in the next delta."""
        self.writes += 1
        if hasattr(self.value, "__assign__"):
            self.value.__assign__(value)
        else:
            self.value = value
        self.changed.notify(sc.sc_time(1, sc.SC_NS))


class QueueOwner:
    """Provide FIFO and TLM callback families over one shared sample queue."""

    def __init__(self, values):
        """Retain shared queue state and one stable FIFO event."""
        self.values = list(values)
        self.changed = sc.sc_event()

    def read(self):
        """Consume and return the next FIFO sample."""
        return self.values.pop(0)

    def nb_read(self):
        """Consume the next FIFO sample when available."""
        return (True, self.read()) if self.values else (False, 0)

    def num_available(self):
        """Report the queue depth through the combined FIFO interface."""
        return len(self.values)

    def data_written_event(self):
        """Borrow the same notification event from all FIFO views."""
        return self.changed

    def get(self):
        """Consume the next sample through TLM blocking get."""
        return self.read()

    def peek(self):
        """Return the next sample without changing shared queue state."""
        return self.values[0]

    def nb_get(self):
        """Consume the next TLM sample when available."""
        return self.nb_read()

    def nb_peek(self):
        """Return the next TLM sample without consuming it when available."""
        return (True, self.peek()) if self.values else (False, 0)

    def nb_can_get(self):
        """Report whether a TLM get can complete immediately."""
        return bool(self.values)

    def nb_can_peek(self):
        """Report whether a TLM peek can complete immediately."""
        return bool(self.values)


cppyy.cppdef(
    """
    #include <cstdint>
    /** Return a borrowed interface's native address for identity assertions.
     * The caller retains the channel, and this helper invokes no Python code.
     */
    std::uintptr_t interface_identity_address(sc_core::sc_interface* value) {
        return reinterpret_cast<std::uintptr_t>(value);
    }
    """
)

name = sc.sc_module_name("interface_identity_matrix")
top = sc.sc_module(name)

wide_type = sc_dt.sc_biguint[137]
wide_high_value = (1 << 136) | 29
signal_cases = (("int", int, 31), ("long_long", "long long", 37), ("wide137", wide_type, wide_high_value))
fifo_cases = (
    ("int", int, (41, 43)),
    ("long_long", "long long", (51, 53)),
    ("wide137", wide_type, ((1 << 136) | 61, (1 << 136) | 63)),
)
tlm_cases = (
    ("int", int, (71, 73)),
    ("long_long", "long long", (81, 83)),
    ("wide137", wide_type, ((1 << 136) | 91, (1 << 136) | 93)),
)

signal_groups = []
for label, value_type, expected in signal_cases:
    for order_name, contracts in (
        ("combined_first", (sc.sc_signal_inout_if, sc.sc_signal_in_if, sc.sc_signal_write_if)),
        ("base_first", (sc.sc_signal_write_if, sc.sc_signal_in_if, sc.sc_signal_inout_if)),
    ):
        owner = SignalOwner(value_type, 7)
        ports = []
        for index, contract in enumerate(contracts):
            port = sc.sc_port[contract[value_type]](f"signal_{label}_{order_name}_{index}")
            port(owner)
            ports.append(port)
        signal_groups.append((label, value_type, expected, order_name, owner, ports))

fifo_groups = []
for label, value_type, samples in fifo_cases:
    for order_name, contracts in (
        ("combined_first", (sc.sc_fifo_in_if, sc.sc_fifo_blocking_in_if, sc.sc_fifo_nonblocking_in_if)),
        ("base_first", (sc.sc_fifo_nonblocking_in_if, sc.sc_fifo_blocking_in_if, sc.sc_fifo_in_if)),
    ):
        native_samples = tuple(make_sample(value_type, value) for value in samples)
        owner = QueueOwner(native_samples)
        ports = []
        for index, contract in enumerate(contracts):
            port = sc.sc_port[contract[value_type]](f"fifo_{label}_{order_name}_{index}")
            port(owner)
            ports.append(port)
        fifo_groups.append((label, samples, order_name, owner, ports))

tlm_groups = []
tlm_contracts = (
    tlm.tlm_get_peek_if,
    tlm.tlm_blocking_get_if,
    tlm.tlm_nonblocking_get_if,
    tlm.tlm_blocking_peek_if,
    tlm.tlm_nonblocking_peek_if,
)
for label, value_type, samples in tlm_cases:
    for order_name, contracts in (
        ("combined_first", tlm_contracts),
        ("base_first", tuple(reversed(tlm_contracts))),
    ):
        native_samples = tuple(make_sample(value_type, value) for value in samples)
        owner = QueueOwner(native_samples)
        ports = []
        for index, contract in enumerate(contracts):
            port = sc.sc_port[contract[value_type]](f"tlm_{label}_{order_name}_{index}")
            port(owner)
            ports.append(port)
        tlm_groups.append((label, samples, order_name, owner, ports))

# Different FIFO template arguments must get different native adapters while
# callback execution still mutates the same Python-owned queue.
shared_owner = QueueOwner((101, 103, 107))
shared_type_ports = []
for label, value_type in (("int", int), ("long_long", "long long"), ("wide137", wide_type)):
    port = sc.sc_port[sc.sc_fifo_in_if[value_type]](f"shared_{label}")
    port(shared_owner)
    shared_type_ports.append((label, port))

del name

signal_callbacks = []
for label, value_type, expected, order_name, owner, ports in signal_groups:
    interfaces = [port.get_interface() for port in ports]
    addresses = [cppyy.gbl.interface_identity_address(interface) for interface in interfaces]
    assert len(set(addresses)) == 1, (label, order_name, addresses)
    event_interfaces = [interface for interface in interfaces if hasattr(interface, "value_changed_event")]
    event_addresses = [cppyy.addressof(interface.value_changed_event()) for interface in event_interfaces]
    assert len(set(event_addresses)) == 1, (label, order_name, event_addresses)
    reader = interfaces[1]
    writer = interfaces[0]
    changed = reader.value_changed_event()

    def record_signal(label=label, order_name=order_name, owner=owner):
        """Record the value and simulation time seen by the inherited event."""
        signal_callbacks.append((label, order_name, value_of(owner.read()), str(sc.sc_time_stamp())))

    top.sc_method(f"observe_signal_{label}_{order_name}", record_signal)
    top.sensitive(changed)
    top.dont_initialize()
    sample = make_sample(value_type, expected)
    writer.write(sample)
    assert owner.writes == 1, (label, order_name, owner.writes)
    actual = reader.read()
    if hasattr(actual, "to_uint64"):
        assert actual.to_uint64() == expected & ((1 << 64) - 1), (label, order_name, actual.to_uint64(), expected)
        assert bool(actual[136]) == bool(expected & (1 << 136)), (label, order_name, actual, expected)
    else:
        assert actual == expected, (label, order_name, actual, expected)

fifo_callbacks = []
for label, samples, order_name, owner, ports in fifo_groups:
    interfaces = [port.get_interface() for port in ports]
    addresses = [cppyy.gbl.interface_identity_address(interface) for interface in interfaces]
    assert len(set(addresses)) == 1, (label, order_name, addresses)
    event_interfaces = [interface for interface in interfaces if hasattr(interface, "data_written_event")]
    events = [interface.data_written_event() for interface in event_interfaces]
    event_addresses = [cppyy.addressof(event) for event in events]
    assert len(set(event_addresses)) == 1, (label, order_name, event_addresses)
    combined = interfaces[0] if order_name == "combined_first" else interfaces[-1]
    blocking = interfaces[0] if order_name == "combined_first" else interfaces[1]
    first = blocking.read()
    assert_sample(first, samples[0])
    assert combined.num_available() == 1
    owner.changed.notify(sc.sc_time(1, sc.SC_NS))

    def record_fifo(label=label, order_name=order_name, owner=owner):
        """Record queue depth and simulation time after its borrowed event."""
        fifo_callbacks.append((label, order_name, owner.num_available(), str(sc.sc_time_stamp())))

    top.sc_method(f"observe_fifo_{label}_{order_name}", record_fifo)
    top.sensitive(events[0])
    top.dont_initialize()

tlm_callbacks = []
for label, samples, order_name, owner, ports in tlm_groups:
    interfaces = [port.get_interface() for port in ports]
    addresses = [cppyy.gbl.interface_identity_address(interface) for interface in interfaces]
    assert len(set(addresses)) == 1, (label, order_name, addresses)
    get_interfaces = [interface for interface in interfaces if hasattr(interface, "ok_to_get")]
    peek_interfaces = [interface for interface in interfaces if hasattr(interface, "ok_to_peek")]
    get_events = [interface.ok_to_get() for interface in get_interfaces]
    peek_events = [interface.ok_to_peek() for interface in peek_interfaces]
    get_addresses = [cppyy.addressof(event) for event in get_events]
    peek_addresses = [cppyy.addressof(event) for event in peek_events]
    assert len(set(get_addresses)) == 1, (label, order_name, get_addresses)
    assert len(set(peek_addresses)) == 1, (label, order_name, peek_addresses)
    assert get_addresses[0] != peek_addresses[0], (label, order_name)
    combined = interfaces[0] if order_name == "combined_first" else interfaces[-1]
    blocking_get = interfaces[1] if order_name == "combined_first" else interfaces[-2]
    blocking_peek = interfaces[-2] if order_name == "combined_first" else interfaces[1]
    assert_sample(blocking_peek.peek(), samples[0])
    assert_sample(blocking_get.get(), samples[0])
    assert_sample(blocking_peek.peek(), samples[1])
    assert combined.nb_can_get() and combined.nb_can_peek()
    combined.notify_ready(sc.sc_time(1, sc.SC_NS))

    def record_tlm(label=label, order_name=order_name, owner=owner):
        """Record shared TLM state and time after inherited readiness events."""
        tlm_callbacks.append((label, order_name, owner.num_available(), str(sc.sc_time_stamp())))

    top.sc_method(f"observe_tlm_get_{label}_{order_name}", record_tlm)
    top.sensitive(get_events[0])
    top.dont_initialize()
    top.sc_method(f"observe_tlm_peek_{label}_{order_name}", record_tlm)
    top.sensitive(peek_events[0])
    top.dont_initialize()

shared_addresses = [cppyy.gbl.interface_identity_address(port.get_interface()) for _, port in shared_type_ports]
assert len(set(shared_addresses)) == len(shared_type_ports), shared_addresses
assert [value_of(port.get_interface().read()) for _, port in shared_type_ports] == [101, 103, 107]
assert shared_owner.values == []

sc.sc_start(3, sc.SC_NS)
expected_signal_callbacks = [
    (label, order_name, expected & ((1 << 64) - 1), "1 ns")
    for label, _, expected in signal_cases
    for order_name in ("combined_first", "base_first")
]
assert sorted(signal_callbacks) == sorted(expected_signal_callbacks), signal_callbacks
expected_fifo_callbacks = [
    (label, order_name, 1, "1 ns") for label, _, _ in fifo_cases for order_name in ("combined_first", "base_first")
]
assert sorted(fifo_callbacks) == sorted(expected_fifo_callbacks), fifo_callbacks
expected_tlm_callbacks = [
    (label, order_name, 1, "1 ns")
    for label, _, _ in tlm_cases
    for order_name in ("combined_first", "base_first")
    for _ in range(2)
]
assert sorted(tlm_callbacks) == sorted(expected_tlm_callbacks), tlm_callbacks
print("IDENTITY_MATRIX_SIGNAL", len(signal_callbacks), "1 ns")
print("IDENTITY_MATRIX_FIFO", len(fifo_callbacks), "1 ns")
print("IDENTITY_MATRIX_TLM", len(tlm_callbacks), "1 ns")
print("IDENTITY_MATRIX_SPECIALIZATIONS", len(set(shared_addresses)), "101 103 107")
sc.sc_stop()
print("INTERFACE_IDENTITY_MATRIX_OK")
