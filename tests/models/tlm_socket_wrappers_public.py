"""Compare Python-visible native socket templates with an independent reference."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc, tlm_utils
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/tlm_socket_wrappers_probe.h"))

probe_type = cppyy.gbl.TlmSocketWrapperProbe
protocol_type = cppyy.gbl.TlmSocketWrapperProtocol
probe = probe_type(sc.sc_module_name("top"))

socket_families = {
    "simple_initiator_socket": ("simple_initiator_default", "simple_initiator_named"),
    "simple_initiator_socket_optional": ("simple_initiator_optional_default", "simple_initiator_optional_named"),
    "simple_initiator_socket_tagged": ("simple_initiator_tagged_default", "simple_initiator_tagged_named"),
    "simple_initiator_socket_tagged_optional": (
        "simple_initiator_tagged_optional_default",
        "simple_initiator_tagged_optional_named",
    ),
    "simple_target_socket": ("simple_target_default", "simple_target_named"),
    "simple_target_socket_optional": ("simple_target_optional_default", "simple_target_optional_named"),
    "simple_target_socket_tagged": ("simple_target_tagged_default", "simple_target_tagged_named"),
    "simple_target_socket_tagged_optional": (
        "simple_target_tagged_optional_default",
        "simple_target_tagged_optional_named",
    ),
    "passthrough_target_socket": ("passthrough_target_default", "passthrough_target_named"),
    "passthrough_target_socket_optional": (
        "passthrough_target_optional_default",
        "passthrough_target_optional_named",
    ),
    "passthrough_target_socket_tagged": (
        "passthrough_target_tagged_default",
        "passthrough_target_tagged_named",
    ),
    "passthrough_target_socket_tagged_optional": (
        "passthrough_target_tagged_optional_default",
        "passthrough_target_tagged_optional_named",
    ),
    "multi_passthrough_initiator_socket_optional": ("multi_initiator_default", "multi_initiator_named"),
    "multi_passthrough_target_socket_optional": ("multi_target_default", "multi_target_named"),
}

for family, members in socket_families.items():
    # Check exposed specialization and both native constructor overloads.
    socket_type = getattr(tlm_utils, family)[probe_type, 73, protocol_type]
    for member in members:
        socket = getattr(probe, member)
        assert type(socket) is socket_type, (family, member, type(socket), socket_type)
    assert str(getattr(probe, members[1]).name()).endswith(members[1])

sc.sc_start(4, sc.SC_NS)
assert probe.transport_complete and len(probe.records) == 3
blocking, named, nonblocking = probe.records[0], probe.records[1], probe.records[2]
assert (int(blocking.address), int(blocking.low_value), bool(blocking.high_bit), str(blocking.time)) == (
    21,
    23,
    False,
    "2 ns",
)
assert (int(named.address), int(named.low_value), bool(named.high_bit), str(named.time)) == (21, 23, False, "3 ns")
assert (int(nonblocking.phase), str(nonblocking.time)) == (int(cppyy.gbl.tlm.BEGIN_RESP), "3 ns")
print(f"SOCKET_WRAPPER_BLOCK_DEFAULT {blocking.address} {blocking.low_value} {int(blocking.high_bit)} {blocking.time}")
print(f"SOCKET_WRAPPER_BLOCK_NAMED {named.address} {named.low_value} {int(named.high_bit)} {named.time}")
print(f"SOCKET_WRAPPER_PHASE {nonblocking.phase} {nonblocking.time}")
print("SOCKET_WRAPPERS_PYTHON_OK")
sc.sc_stop()
