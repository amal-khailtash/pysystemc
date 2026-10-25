"""Exercise raw simple socket `_b` APIs and retained Python callback adapters."""

from pathlib import Path
import gc
import weakref

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/tlm_utils_simple_b_probe.h"))

OWNER = cppyy.gbl.SimpleBNativeOwner
PAYLOAD = cppyy.gbl.SimpleBPayload
PROTOCOL = cppyy.gbl.SimpleBProtocol
FAMILIES = (
    ("simple_initiator_socket_b", "initiator_default", "initiator_named", tlm.tlm_initiator_socket),
    (
        "simple_initiator_socket_tagged_b",
        "tagged_initiator_default",
        "tagged_initiator_named",
        tlm.tlm_initiator_socket,
    ),
    ("simple_target_socket_b", "target_default", "target_named", tlm.tlm_target_socket),
    ("simple_target_socket_tagged_b", "tagged_target_default", "tagged_target_named", tlm.tlm_target_socket),
)

module_name = sc.sc_module_name("top")
owner = OWNER(module_name)
del module_name
assert int(OWNER.live_count) == 1

for family, default_member, named_member, base_template in FAMILIES:
    # Check aliases, constructors and each family’s declared template type.
    socket_type = getattr(tlm_utils, family)[OWNER, 73, PROTOCOL, sc.SC_ONE_OR_MORE_BOUND]
    if family == "simple_target_socket_b":
        default_socket, named_socket = owner.plain_target_base(0), owner.plain_target_base(1)
    elif family == "simple_target_socket_tagged_b":
        default_socket, named_socket = owner.tagged_target_base(0), owner.tagged_target_base(1)
    else:
        default_socket, named_socket = getattr(owner, default_member), getattr(owner, named_member)
    assert isinstance(default_socket, socket_type), (family, type(default_socket), socket_type)
    assert isinstance(named_socket, socket_type), (family, type(named_socket), socket_type)
    assert socket_type.transaction_type == PAYLOAD
    assert socket_type.phase_type == tlm.tlm_phase
    assert socket_type.sync_enum_type is int
    assert socket_type.fw_interface_type == tlm.tlm_fw_transport_if[PROTOCOL]
    assert socket_type.bw_interface_type == tlm.tlm_bw_transport_if[PROTOCOL]
    expected_base = base_template[73, PROTOCOL, 1, sc.SC_ONE_OR_MORE_BOUND]
    assert socket_type.base_type == expected_base
    assert str(socket_type.default_name()).startswith(family.removesuffix("_b"))
    assert default_socket.kind()
    assert str(named_socket.name()).endswith(named_member)

payloads = []
forward_interfaces = []
backward_interfaces = []
for token in range(4):
    # Create caller-owned wide payloads and direct native transport interfaces.
    payload = PAYLOAD()
    payload.token = token
    payload.value = cppyy.gbl.sc_dt.sc_biguint[137](f"0x1{'0' * 32}{17 + token:02x}")
    payload.set_address(17 + token)
    payloads.append(payload)
    forward = owner.forward_interface(token)
    backward = owner.backward_interface(token)
    forward.b_transport.__release_gil__ = True
    forward_interfaces.append(forward)
    backward_interfaces.append(backward)

for token, payload in enumerate(payloads):
    # Invoke every forward/backward interface operation from Python.
    forward = forward_interfaces[token]
    backward = backward_interfaces[token]
    phase = tlm.tlm_phase(tlm.BEGIN_REQ)
    delay = sc.sc_time(0, sc.SC_NS)
    assert forward.nb_transport_fw(payload, phase, delay) == tlm.TLM_UPDATED
    assert phase == tlm.BEGIN_RESP and delay == sc.sc_time(1, sc.SC_NS)
    assert forward.transport_dbg(payload) == 32 + token
    dmi = tlm.tlm_dmi()
    assert forward.get_direct_mem_ptr(payload, dmi)
    assert (dmi.get_start_address(), dmi.get_end_address()) == (16 + token * 16, 31 + token * 16)
    assert dmi.is_read_allowed() and dmi.is_write_allowed()
    assert dmi.get_read_latency() == sc.sc_time(1, sc.SC_NS)
    assert dmi.get_write_latency() == sc.sc_time(2, sc.SC_NS)
    dmi_pointer = dmi.get_dmi_ptr()
    dmi_pointer.reshape((16,))
    dmi_pointer[3] = 80 + token
    assert ord(owner.memory[token * 16 + 3]) == 80 + token
    phase.__assign__(tlm.BEGIN_RESP)
    assert backward.nb_transport_bw(payload, phase, delay) == tlm.TLM_COMPLETED
    assert phase == tlm.END_RESP
    first = 0x1000000000000000 + token
    last = 0xF000000000000000 + token
    backward.invalidate_direct_mem_ptr(first, last)


factory_name = sc.sc_module_name("factory_top")
factory_top = sc.sc_module(factory_name)


class PythonFactoryTarget:
    """Retain custom-protocol Python callbacks for the public socket factory."""

    def __init__(self):
        """Create callback records and native DMI storage for the socket lifetime."""
        self.records = []
        self.cleanup = []
        self.memory = cppyy.gbl.std.vector["unsigned char"](16, 0)

    def nonblocking(self, transaction, phase, delay):
        """Update a borrowed phase and record the native callback time."""
        self.records.append(("fw", int(transaction.token), str(sc.sc_time_stamp())))
        phase.__assign__(tlm.BEGIN_RESP)
        delay.__assign__(sc.sc_time(1, sc.SC_NS))
        return tlm.TLM_UPDATED

    def debug(self, transaction):
        """Return a stable debug count for the retained callback path."""
        self.records.append(("debug", int(transaction.token), str(sc.sc_time_stamp())))
        return 47

    def dmi(self, transaction, result):
        """Grant borrowed memory and explicit range/permission metadata."""
        self.records.append(("dmi", int(transaction.token), str(sc.sc_time_stamp())))
        result.set_start_address(17)
        result.set_end_address(32)
        result.set_dmi_ptr(self.memory.data())
        result.allow_read_write()
        return True

    def blocking(self, transaction, delay):
        """Wait so process reset and kill exercise Python callback cleanup."""
        try:
            sc.wait(delay)
        finally:
            self.cleanup.append(str(sc.sc_time_stamp()))


class PythonFactoryInitiator:
    """Retain backward callbacks on the native Python socket factory."""

    def __init__(self):
        """Create a record list for backward traffic and invalidations."""
        self.records = []

    def backward(self, transaction, phase, delay):
        """Complete a borrowed response phase through the Python callback."""
        self.records.append(("bw", int(transaction.token), str(sc.sc_time_stamp())))
        phase.__assign__(tlm.END_RESP)
        return tlm.TLM_COMPLETED

    def invalidate(self, first, last):
        """Record the exact unsigned invalidation interval."""
        self.records.append(("invalidate", int(first), int(last)))


class PythonTaggedFactoryTarget:
    """Record IDs delivered through the tagged Python target factory."""

    def __init__(self):
        """Retain tagged callback observations and a native DMI buffer."""
        self.records = []
        self.memory = cppyy.gbl.std.vector["unsigned char"](16, 0)

    def nonblocking(self, identifier, transaction, phase, delay):
        """Return an updated phase after checking the registered target ID."""
        assert identifier == 31
        self.records.append(("fw", identifier, int(transaction.token)))
        phase.__assign__(tlm.BEGIN_RESP)
        delay.__assign__(sc.sc_time(1, sc.SC_NS))
        return tlm.TLM_UPDATED

    def debug(self, identifier, transaction):
        """Return the tagged debug count after checking its registration ID."""
        assert identifier == 31
        self.records.append(("debug", identifier, int(transaction.token)))
        return 63

    def dmi(self, identifier, transaction, result):
        """Grant tagged DMI into storage retained by the callback owner."""
        assert identifier == 31
        self.records.append(("dmi", identifier, int(transaction.token)))
        result.set_start_address(48)
        result.set_end_address(63)
        result.set_dmi_ptr(self.memory.data())
        result.allow_read_write()
        return True

    def blocking(self, identifier, transaction, delay):
        """Provide the tagged blocking registration required by the factory."""
        assert identifier == 31
        self.records.append(("blocking", identifier, int(transaction.token)))


class PythonTaggedFactoryInitiator:
    """Retain tagged backward callbacks and validate their independent ID."""

    def __init__(self):
        """Create tagged backward callback observations."""
        self.records = []

    def backward(self, identifier, transaction, phase, delay):
        """Complete a tagged response phase and record payload token."""
        assert identifier == 41
        self.records.append(("bw", identifier, int(transaction.token)))
        phase.__assign__(tlm.END_RESP)
        return tlm.TLM_COMPLETED

    def invalidate(self, identifier, first, last):
        """Record tagged invalidation ID and full-width boundaries."""
        assert identifier == 42
        self.records.append(("invalidate", identifier, int(first), int(last)))


factory_target_owner = PythonFactoryTarget()
factory_target_reference = weakref.ref(factory_target_owner)
factory_target = tlm_utils.callback_target_socket("factory_target", width=73, protocol=PROTOCOL)
factory_target.register_b_transport(factory_target_owner.blocking)
factory_target.register_nb_transport_fw(factory_target_owner.nonblocking)
factory_target.register_transport_dbg(factory_target_owner.debug)
factory_target.register_get_direct_mem_ptr(factory_target_owner.dmi)
factory_initiator_owner = PythonFactoryInitiator()
factory_initiator_reference = weakref.ref(factory_initiator_owner)
factory_initiator = tlm_utils.callback_initiator_socket("factory_initiator", width=73, protocol=PROTOCOL)
factory_initiator.register_nb_transport_bw(factory_initiator_owner.backward)
factory_initiator.register_invalidate_direct_mem_ptr(factory_initiator_owner.invalidate)
factory_initiator.bind(factory_target)
tagged_factory_target_owner = PythonTaggedFactoryTarget()
tagged_factory_target_reference = weakref.ref(tagged_factory_target_owner)
tagged_factory_target = tlm_utils.callback_target_socket(
    "tagged_factory_target", width=73, protocol=PROTOCOL, tagged=True
)
tagged_factory_target.register_b_transport(tagged_factory_target_owner.blocking, 31)
tagged_factory_target.register_nb_transport_fw(tagged_factory_target_owner.nonblocking, 31)
tagged_factory_target.register_transport_dbg(tagged_factory_target_owner.debug, 31)
tagged_factory_target.register_get_direct_mem_ptr(tagged_factory_target_owner.dmi, 31)
tagged_factory_initiator_owner = PythonTaggedFactoryInitiator()
tagged_factory_initiator_reference = weakref.ref(tagged_factory_initiator_owner)
tagged_factory_initiator = tlm_utils.callback_initiator_socket(
    "tagged_factory_initiator", width=73, protocol=PROTOCOL, tagged=True
)
tagged_factory_initiator.register_nb_transport_bw(tagged_factory_initiator_owner.backward, 41)
tagged_factory_initiator.register_invalidate_direct_mem_ptr(tagged_factory_initiator_owner.invalidate, 42)
tagged_factory_initiator.bind(tagged_factory_target)
del factory_name

factory_payload = PAYLOAD()
factory_payload.token = 9
factory_payload.value = cppyy.gbl.sc_dt.sc_biguint[137](f"0x1{'0' * 32}11")
factory_payload.set_address(17)
factory_forward = factory_initiator.get_interface()
assert factory_forward.b_transport.__release_gil__
factory_phase = tlm.tlm_phase(tlm.BEGIN_REQ)
factory_delay = sc.sc_time(0, sc.SC_NS)
assert factory_forward.nb_transport_fw(factory_payload, factory_phase, factory_delay) == tlm.TLM_UPDATED
assert factory_phase == tlm.BEGIN_RESP and factory_delay == sc.sc_time(1, sc.SC_NS)
assert factory_forward.transport_dbg(factory_payload) == 47
factory_dmi = tlm.tlm_dmi()
assert factory_forward.get_direct_mem_ptr(factory_payload, factory_dmi)
assert factory_dmi.get_start_address() == 17 and factory_dmi.get_end_address() == 32
factory_backward = factory_target.get_base_port().get_interface()
factory_phase.__assign__(tlm.BEGIN_RESP)
assert factory_backward.nb_transport_bw(factory_payload, factory_phase, factory_delay) == tlm.TLM_COMPLETED
assert factory_phase == tlm.END_RESP
factory_backward.invalidate_direct_mem_ptr(0, 2**64 - 1)
tagged_factory_payload = PAYLOAD()
tagged_factory_payload.token = 10
tagged_factory_payload.value = cppyy.gbl.sc_dt.sc_biguint[137](f"0x1{'0' * 32}1b")
tagged_factory_payload.set_address(27)
tagged_factory_forward = tagged_factory_initiator.get_interface()
tagged_factory_backward = tagged_factory_target.get_base_port().get_interface()
tagged_factory_phase = tlm.tlm_phase(tlm.BEGIN_REQ)
tagged_factory_delay = sc.sc_time(0, sc.SC_NS)
assert (
    tagged_factory_forward.nb_transport_fw(tagged_factory_payload, tagged_factory_phase, tagged_factory_delay)
    == tlm.TLM_UPDATED
)
assert tagged_factory_phase == tlm.BEGIN_RESP and tagged_factory_delay == sc.sc_time(1, sc.SC_NS)
assert tagged_factory_forward.transport_dbg(tagged_factory_payload) == 63
tagged_factory_dmi = tlm.tlm_dmi()
assert tagged_factory_forward.get_direct_mem_ptr(tagged_factory_payload, tagged_factory_dmi)
tagged_factory_phase.__assign__(tlm.BEGIN_RESP)
assert (
    tagged_factory_backward.nb_transport_bw(tagged_factory_payload, tagged_factory_phase, tagged_factory_delay)
    == tlm.TLM_COMPLETED
)
assert tagged_factory_phase == tlm.END_RESP
tagged_factory_backward.invalidate_direct_mem_ptr(0x1000000000000000, 0xF000000000000000)
del factory_target_owner, factory_initiator_owner
del tagged_factory_target_owner, tagged_factory_initiator_owner
gc.collect()
assert factory_target_reference() is not None and factory_initiator_reference() is not None
assert tagged_factory_target_reference() is not None and tagged_factory_initiator_reference() is not None

factory_delay.__assign__(sc.sc_time(2, sc.SC_NS))
factory_returns = []


def factory_blocking_call():
    """Enter a retained callback that controller processes will restart/kill."""
    factory_forward.b_transport(factory_payload, factory_delay)
    factory_returns.append(str(sc.sc_time_stamp()))


factory_handle = sc.sc_spawn(factory_blocking_call, "factory_blocking")


def factory_controller():
    """Reset the suspended Python callback once, then kill its restarted call."""
    sc.wait(1, sc.SC_NS)
    factory_handle.reset()
    sc.wait(1, sc.SC_NS)
    factory_handle.kill()


factory_controller_handle = sc.sc_spawn(factory_controller, "factory_controller")

handles = []
completed = {}
for token, payload in enumerate(payloads):
    # Run waiting b_transport calls as independent SystemC processes.

    def blocking_call(index=token, transaction=payload):
        """Exercise a borrowed payload through the native blocking interface."""
        delay = sc.sc_time(2, sc.SC_NS)
        forward_interfaces[index].b_transport(transaction, delay)
        assert transaction.get_address() == 21 + index
        assert transaction.is_response_ok()
        assert transaction.value.to_uint64() == 23 + index
        assert not transaction.value[136].to_bool()
        assert delay == sc.SC_ZERO_TIME
        completed[index] = str(sc.sc_time_stamp())

    handles.append(sc.sc_spawn(blocking_call, f"blocking_{token}"))

sc.sc_start(3, sc.SC_NS)
assert all(handle.terminated() for handle in handles)
assert completed == dict.fromkeys(range(4), "2 ns"), completed
assert factory_handle.terminated() and factory_controller_handle.terminated()
assert factory_returns == [] and factory_target_reference().cleanup == ["1 ns", "2 ns"]
assert factory_target_reference().records == [
    ("fw", 9, "0 s"),
    ("debug", 9, "0 s"),
    ("dmi", 9, "0 s"),
]
assert factory_initiator_reference().records == [("bw", 9, "0 s"), ("invalidate", 0, 2**64 - 1)]
assert tagged_factory_target_reference().records == [
    ("fw", 31, 10),
    ("debug", 31, 10),
    ("dmi", 31, 10),
]
assert tagged_factory_initiator_reference().records == [
    ("bw", 41, 10),
    ("invalidate", 42, 0x1000000000000000, 0xF000000000000000),
]
assert owner.records.size() == 24

for record in sorted(owner.records, key=lambda item: (int(item.token), int(item.kind))):
    # Verify native callback identity, tagged IDs, phases, wide values and time.
    token = int(record.token)
    kind = int(record.kind)
    callback_id = int(record.callback_id)
    identity = kind != int(OWNER.invalidate)
    if identity:
        assert int(record.payload_address) == cppyy.addressof(payloads[token])
    assert callback_id == (200 + token if kind in (int(OWNER.backward_nb), int(OWNER.invalidate)) else 100 + token)
    if kind == int(OWNER.forward_block):
        assert (int(record.address), int(record.low_value), bool(record.high_bit), str(record.time)) == (
            21 + token,
            23 + token,
            False,
            "2 ns",
        )
    else:
        assert str(record.time) == "0 s"
        if kind == int(OWNER.forward_nb):
            assert int(record.phase) == int(tlm.BEGIN_RESP)
        elif kind == int(OWNER.backward_nb):
            assert int(record.phase) == int(tlm.END_RESP)
        elif kind == int(OWNER.invalidate):
            assert int(record.address) == 0x1000000000000000 + token
            assert int(record.low_value) == 0xF000000000000000 + token
    print(
        "SIMPLE_B_REC|"
        + "|".join(
            map(
                str,
                (
                    token,
                    callback_id,
                    kind,
                    int(identity),
                    int(record.address),
                    int(record.low_value),
                    int(record.high_bit),
                    int(record.phase),
                    str(record.time),
                ),
            )
        )
    )

for token, member in enumerate(
    ("initiator_default", "initiator_named", "tagged_initiator_default", "tagged_initiator_named")
):
    # Verify the inherited subport index and target operator-> resolve natively.
    assert cppyy.addressof(getattr(owner, member)[0]) == cppyy.addressof(forward_interfaces[token])
    assert int(owner.backward_operator_address(token)) == cppyy.addressof(backward_interfaces[token])

assert "tlm_initiator_socket" in str(owner.initiator_default.kind())
assert "tlm_target_socket" in str(owner.target_default.kind())
assert cppyy.gbl.SimpleBTarget.start_count == 2
assert cppyy.gbl.SimpleBTaggedTarget.start_count == 2
sc.sc_stop()
handles.clear()
completed.clear()
forward_interfaces.clear()
backward_interfaces.clear()
del (
    forward,
    backward,
    payload,
    dmi,
    dmi_pointer,
    record,
    socket_type,
    expected_base,
    default_socket,
    named_socket,
    owner,
)
payloads.clear()
gc.collect()
assert int(OWNER.live_count) == 0
print("SIMPLE_B_NATIVE_OWNER_OK")
