"""Exercise the reviewed public TLM socket API matrix against native results."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc, tlm
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/tlm_native_sockets_probe.h"))

OWNER = cppyy.gbl.TlmNativeSocketsOwner
PAYLOAD = cppyy.gbl.TlmNativeSocketsPayload
PROTOCOL = cppyy.gbl.TlmNativeSocketsProtocol
FORWARD = cppyy.gbl.TlmNativeSocketsForward
BACKWARD = cppyy.gbl.TlmNativeSocketsBackward
BASE_INITIATOR = cppyy.gbl.TlmNativeBaseInitiator
BASE_TARGET = cppyy.gbl.TlmNativeBaseTarget
POLICY = sc.SC_ZERO_OR_MORE_BOUND
PROTOCOL_INDEX = OWNER.protocol_index()


def check_socket(socket, family, category, kind, named_suffix):
    """Check common native metadata and retain each socket's declared name.

    Args:
        socket: Native socket instance owned by a live SystemC module.
        family: Expected C++ kind string.
        category: Expected native socket category enum.
        kind: Whether this is the initiator or target API family.
        named_suffix: Optional expected ending for an explicit socket name.

    Returns:
        One portable metadata line used for the independent native comparison.
    """
    assert socket.get_bus_width() == 73
    assert int(socket.get_socket_category()) == int(category)
    assert socket.kind() == family
    assert socket.get_protocol_types() == PROTOCOL_INDEX
    assert isinstance(socket, tlm.tlm_base_socket_if)
    if named_suffix:
        assert str(socket.name()).endswith(named_suffix)
    count = socket.get_base_port().size()
    if kind == "target":
        assert socket.size() == count
    return f"SOCKET_NATIVE_REC|{family}|73|{int(category)}|{count}|{socket.kind()}"


def check_interface_accessors(socket, port_interface, export_interface):
    """Check native reference returns through Python's concrete proxy layer.

    Args:
        socket: Bound concrete or base socket whose getters are inspected.
        port_interface: Forward or backward interface used by the socket port.
        export_interface: Forward or backward interface provided by its export.

    Returns:
        None. Every getter must return a live proxy of its declared interface.
    """
    port = socket.get_base_port()
    export = socket.get_base_export()
    interface = socket.get_base_interface()
    assert isinstance(port, sc.sc_port_b[port_interface])
    assert isinstance(export, sc.sc_export[export_interface])
    assert isinstance(interface, export_interface)


def apply_python_bind_matrix(bindings):
    """Exercise every public bind and call overload with native interfaces.

    Args:
        bindings: Module that owns the endpoints and every independent socket.

    Returns:
        None. Successful binding proves cppyy selected all native overloads.
    """
    endpoint = bindings.endpoint
    bindings.initiator_interface_bind.bind(endpoint)
    bindings.initiator_interface_call(endpoint)
    bindings.target_interface_bind.bind(endpoint)
    bindings.target_interface_call(endpoint)

    bindings.target_socket_bind.bind(endpoint)
    bindings.initiator_target_bind.bind(endpoint)
    bindings.initiator_target_bind.bind(bindings.target_socket_bind)
    bindings.target_socket_call.bind(endpoint)
    bindings.initiator_target_call.bind(endpoint)
    bindings.initiator_target_call(bindings.target_socket_call)

    bindings.initiator_hierarchy_bind_target.bind(endpoint)
    bindings.initiator_hierarchy_bind_parent.bind(endpoint)
    bindings.initiator_hierarchy_bind_parent.bind(bindings.initiator_hierarchy_bind_child)
    bindings.initiator_hierarchy_bind_child.bind(bindings.initiator_hierarchy_bind_target)
    bindings.initiator_hierarchy_call_target.bind(endpoint)
    bindings.initiator_hierarchy_call_parent.bind(endpoint)
    bindings.initiator_hierarchy_call_parent(bindings.initiator_hierarchy_call_child)
    bindings.initiator_hierarchy_call_child(bindings.initiator_hierarchy_call_target)

    bindings.target_initiator_bind_socket.bind(endpoint)
    bindings.target_initiator_bind.bind(endpoint)
    bindings.target_initiator_bind_socket.bind(bindings.target_initiator_bind)
    bindings.target_initiator_call_socket.bind(endpoint)
    bindings.target_initiator_call.bind(endpoint)
    bindings.target_initiator_call_socket(bindings.target_initiator_call)

    bindings.target_hierarchy_bind_initiator.bind(endpoint)
    bindings.target_hierarchy_bind_child.bind(endpoint)
    bindings.target_hierarchy_bind_parent.bind(bindings.target_hierarchy_bind_child)
    bindings.target_hierarchy_bind_initiator.bind(bindings.target_hierarchy_bind_parent)
    bindings.target_hierarchy_call_initiator.bind(endpoint)
    bindings.target_hierarchy_call_child.bind(endpoint)
    bindings.target_hierarchy_call_parent(bindings.target_hierarchy_call_child)
    bindings.target_hierarchy_call_initiator(bindings.target_hierarchy_call_parent)


def run_socket_probe():
    """Reflect aliases, bindings, interface cv overloads and callbacks.

    Returns:
        Portable metadata records matching the independent native process.

    Raises:
        AssertionError: If cppyy cannot expose a reviewed socket declaration or
            a native getter, binding overload, callback or lifetime invariant.
    """
    owner = OWNER(sc.sc_module_name("top"))
    binding = owner.bindings
    assert int(OWNER.live_count) == 1
    for socket in (
        owner.base_initiator_default,
        owner.base_initiator_named,
        owner.base_target_default,
        owner.base_target_named,
        owner.initiator_default,
        owner.initiator_named,
        owner.target_default,
        owner.target_named,
    ):
        socket.bind(owner.endpoint)
    for socket in (
        owner.base_initiator_default,
        owner.base_initiator_named,
        owner.initiator_default,
        owner.initiator_named,
    ):
        check_interface_accessors(socket, FORWARD, BACKWARD)
    for socket in (owner.base_target_default, owner.base_target_named, owner.target_default, owner.target_named):
        check_interface_accessors(socket, BACKWARD, FORWARD)

    # Specializations use the custom protocol, 73-bit bus and two-port policy.
    base_initiator_b = tlm.tlm_base_initiator_socket_b[73, FORWARD, BACKWARD]
    base_target_b = tlm.tlm_base_target_socket_b[73, FORWARD, BACKWARD]
    base_initiator = tlm.tlm_base_initiator_socket[73, FORWARD, BACKWARD, 2, POLICY]
    base_target = tlm.tlm_base_target_socket[73, FORWARD, BACKWARD, 2, POLICY]
    initiator = tlm.tlm_initiator_socket[73, PROTOCOL, 2, POLICY]
    target = tlm.tlm_target_socket[73, PROTOCOL, 2, POLICY]
    assert type(owner.base_initiator_default) is base_initiator
    assert type(owner.base_target_default) is base_target
    assert type(owner.initiator_default) is initiator
    assert type(owner.target_default) is target
    assert isinstance(owner.base_initiator_default, base_initiator_b)
    assert isinstance(owner.base_initiator_default, tlm.tlm_base_socket_if)
    assert isinstance(owner.base_target_default, base_target_b)
    assert isinstance(owner.base_target_default, tlm.tlm_base_socket_if)
    assert base_initiator.fw_interface_type == FORWARD
    assert base_initiator.bw_interface_type == BACKWARD
    assert base_initiator.port_type == sc.sc_port[FORWARD, 2, POLICY]
    assert base_initiator.export_type == sc.sc_export[BACKWARD]
    assert base_initiator.base_target_socket_type == base_target_b
    assert base_initiator.base_type == base_initiator_b
    assert base_target.fw_interface_type == FORWARD
    assert base_target.bw_interface_type == BACKWARD
    assert base_target.port_type == sc.sc_port[BACKWARD, 2, POLICY]
    assert base_target.export_type == sc.sc_export[FORWARD]
    assert base_target.base_initiator_socket_type == base_initiator_b
    assert base_target.base_type == base_target_b

    rows = [
        check_socket(
            owner.base_initiator_default,
            "tlm_base_initiator_socket",
            tlm.TLM_INITIATOR_SOCKET,
            "initiator",
            "",
        ),
        check_socket(
            owner.base_target_default,
            "tlm_base_target_socket",
            tlm.TLM_TARGET_SOCKET,
            "target",
            "",
        ),
        check_socket(
            owner.initiator_default,
            "tlm_initiator_socket",
            tlm.TLM_INITIATOR_SOCKET,
            "initiator",
            "",
        ),
        check_socket(
            owner.target_default,
            "tlm_target_socket",
            tlm.TLM_TARGET_SOCKET,
            "target",
            "",
        ),
    ]
    rows.extend(
        [
            check_socket(
                owner.base_initiator_named,
                "tlm_base_initiator_socket",
                tlm.TLM_INITIATOR_SOCKET,
                "initiator",
                "base_initiator_named",
            ),
            check_socket(
                owner.base_target_named,
                "tlm_base_target_socket",
                tlm.TLM_TARGET_SOCKET,
                "target",
                "base_target_named",
            ),
            check_socket(
                owner.initiator_named,
                "tlm_initiator_socket",
                tlm.TLM_INITIATOR_SOCKET,
                "initiator",
                "initiator_named",
            ),
            check_socket(
                owner.target_named,
                "tlm_target_socket",
                tlm.TLM_TARGET_SOCKET,
                "target",
                "target_named",
            ),
        ]
    )

    # Verify abstract base cv overloads resolve to each concrete subobject.
    for socket in (owner.base_initiator_default, owner.initiator_default):
        base_address = cppyy.addressof(socket)
        assert int(OWNER.interface_port(socket)) == cppyy.addressof(socket.get_base_port())
        assert int(OWNER.const_interface_port(socket)) == cppyy.addressof(socket.get_base_port())
        assert int(OWNER.interface_export(socket)) == cppyy.addressof(socket.get_base_export())
        assert int(OWNER.const_interface_export(socket)) == cppyy.addressof(socket.get_base_export())
        assert base_address != 0
    for socket in (owner.base_target_default, owner.target_default):
        base_address = cppyy.addressof(socket)
        assert int(OWNER.interface_port(socket)) == cppyy.addressof(socket.get_base_port())
        assert int(OWNER.const_interface_port(socket)) == cppyy.addressof(socket.get_base_port())
        assert int(OWNER.interface_export(socket)) == cppyy.addressof(socket.get_base_export())
        assert int(OWNER.const_interface_export(socket)) == cppyy.addressof(socket.get_base_export())
        assert base_address != 0
    assert int(OWNER.const_initiator_port(owner.base_initiator_default)) == cppyy.addressof(
        owner.base_initiator_default.get_base_port()
    )
    assert int(OWNER.const_initiator_export(owner.base_initiator_default)) == cppyy.addressof(
        owner.base_initiator_default.get_base_export()
    )
    assert int(OWNER.const_initiator_interface(owner.base_initiator_default)) == int(
        OWNER.initiator_interface(owner.base_initiator_default)
    )
    assert int(OWNER.const_target_port(owner.base_target_default)) == cppyy.addressof(
        owner.base_target_default.get_base_port()
    )
    assert int(OWNER.const_target_export(owner.base_target_default)) == cppyy.addressof(
        owner.base_target_default.get_base_export()
    )
    assert int(OWNER.const_target_interface(owner.base_target_default)) == int(
        OWNER.target_interface(owner.base_target_default)
    )

    # Each native overload is called independently through cppyy.
    apply_python_bind_matrix(binding)
    target_socket = binding.target_socket_bind
    assert target_socket.size() == target_socket.get_base_port().size() == 1
    assert int(OWNER.target_arrow(target_socket)) == int(OWNER.target_index(target_socket, 0))
    indexed_interface = target_socket[0]
    assert cppyy.addressof(indexed_interface) == int(OWNER.target_index(target_socket, 0))

    payload = PAYLOAD()
    payload.set_address(37)
    payload.value = cppyy.gbl.sc_dt.sc_biguint[137](f"0x1{'0' * 32}a5")
    assert payload.value[136].to_bool() and payload.value.to_uint64() == 0xA5
    forward = binding.initiator_target_bind.get_base_port().get_interface()
    backward = target_socket[0]
    phase = tlm.tlm_phase(tlm.BEGIN_REQ)
    delay = sc.sc_time(0, sc.SC_NS)
    assert forward.nb_transport_fw(payload, phase, delay) == tlm.TLM_UPDATED
    assert phase == tlm.BEGIN_RESP
    assert forward.b_transport(payload, delay) is None
    dmi = tlm.tlm_dmi()
    assert not forward.get_direct_mem_ptr(payload, dmi)
    assert forward.transport_dbg(payload) == 0
    phase.__assign__(tlm.BEGIN_RESP)
    assert backward.nb_transport_bw(payload, phase, delay) == tlm.TLM_COMPLETED
    assert phase == tlm.END_RESP
    backward.invalidate_direct_mem_ptr(0x1000000000000000, 0xF000000000000000)
    assert binding.endpoint.calls == 6
    assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
    rows.append(f"SOCKET_BIND_REC|{target_socket.size()}|{int(phase)}|{binding.endpoint.calls}")

    sc.sc_start(1, sc.SC_NS)
    assert sc.sc_time_stamp() == sc.sc_time(1, sc.SC_NS)
    sc.sc_stop()
    return rows


assert int(OWNER.live_count) == 0
for row in run_socket_probe():
    print(row)
assert int(OWNER.live_count) == 0
print("TLM_NATIVE_SOCKETS_PYTHON_OK")
