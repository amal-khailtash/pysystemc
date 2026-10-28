"""Exercise native passthrough B sockets through direct Python interface calls."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils

import cppyy


PROBE = Path(__file__).resolve().parents[1] / "native/tlm_utils_passthrough_b_probe.h"
cppyy.include(str(PROBE))
probe = cppyy.gbl.tlm_utils_passthrough_b_probe
module_type = probe.Module
plain_type = module_type.plain_socket_type
tagged_type = module_type.tagged_socket_type


def cpp_name(native_type):
    """Return the concrete C++ spelling recorded by cppyy."""
    return native_type.__cpp_name__


# Exercise all six public aliases on each concrete B socket specialization.
for socket_type in (plain_type, tagged_type):
    for alias in (
        "transaction_type",
        "phase_type",
        "sync_enum_type",
        "fw_interface_type",
        "bw_interface_type",
        "base_type",
    ):
        assert hasattr(socket_type, alias)
    assert cpp_name(socket_type.transaction_type).endswith("Payload137")
    assert "tlm_phase" in cpp_name(socket_type.phase_type)
    assert socket_type.sync_enum_type is int
    assert "tlm_fw_transport_if" in cpp_name(socket_type.fw_interface_type)
    assert "tlm_bw_transport_if" in cpp_name(socket_type.bw_interface_type)
    assert "tlm_target_socket" in cpp_name(socket_type.base_type)
    assert str(socket_type.default_name()).startswith("passthrough_target_socket")
assert str(tagged_type.default_name()).startswith("passthrough_target_socket_tagged")

assert hasattr(tlm_utils, "passthrough_target_socket_b")
assert hasattr(tlm_utils, "passthrough_target_socket_tagged_b")
owner = module_type(sc.sc_module_name("top"))
assert isinstance(owner.plain_target, plain_type)
assert isinstance(owner.tagged_target, tagged_type)
assert owner.plain_target.kind() == "tlm_target_socket"
assert owner.tagged_target.kind() == "tlm_target_socket"

# Bind calls use the native interface and target-socket overloads directly.
owner.plain_initiator.bind(owner)
owner.tagged_initiator.bind(owner)
owner.plain_target.bind(owner.plain_initiator)
owner.tagged_target.bind(owner.tagged_initiator)

plain = owner.plain_target.get_base_interface()
tagged = owner.tagged_target.get_base_interface()
payload = owner.payload
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
delay = sc.sc_time(0, sc.SC_NS)
assert plain.nb_transport_fw(payload, phase, delay) == tlm.TLM_UPDATED
assert phase == tlm.BEGIN_RESP and delay == sc.sc_time(1, sc.SC_NS)
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
delay = sc.sc_time(0, sc.SC_NS)
assert tagged.nb_transport_fw(payload, phase, delay) == tlm.TLM_UPDATED
assert phase == tlm.END_REQ and delay == sc.sc_time(2, sc.SC_NS)

plain_dmi = tlm.tlm_dmi()
assert plain.get_direct_mem_ptr(payload, plain_dmi)
assert (plain_dmi.get_start_address(), plain_dmi.get_end_address()) == (0x1000, 0x10FF)
assert plain_dmi.is_read_allowed() and plain_dmi.is_write_allowed()
assert plain_dmi.get_read_latency() == sc.sc_time(3, sc.SC_NS)
assert plain_dmi.get_write_latency() == sc.sc_time(4, sc.SC_NS)
tagged_dmi = tlm.tlm_dmi()
assert tagged.get_direct_mem_ptr(payload, tagged_dmi)
assert (tagged_dmi.get_start_address(), tagged_dmi.get_end_address()) == (0x2000, 0x20FF)
assert tagged_dmi.is_read_allowed() and not tagged_dmi.is_write_allowed()
assert tagged_dmi.get_read_latency() == sc.sc_time(6, sc.SC_NS)
assert plain.transport_dbg(payload) == 0xF0000011
assert tagged.transport_dbg(payload) == 0xF0000025

# Native target operator[] exposes the borrowed backward interface.
payload.marker = 3
phase = tlm.tlm_phase(tlm.BEGIN_RESP)
delay = sc.sc_time(2, sc.SC_NS)
assert owner.plain_target[0].nb_transport_bw(payload, phase, delay) == tlm.TLM_COMPLETED
assert phase == tlm.END_RESP and delay == sc.sc_time(7, sc.SC_NS)
payload.marker = 4
phase = tlm.tlm_phase(tlm.END_REQ)
delay = sc.sc_time(3, sc.SC_NS)
assert owner.tagged_target[0].nb_transport_bw(payload, phase, delay) == tlm.TLM_COMPLETED
assert phase == tlm.END_RESP and delay == sc.sc_time(8, sc.SC_NS)
payload.marker = 5
owner.plain_target[0].invalidate_direct_mem_ptr(0, 2**64 - 1)
payload.marker = 6
owner.tagged_target[0].invalidate_direct_mem_ptr(2**32, 2**64 - 1)

# Blocking native member callbacks wait on the registered module process.
sc.sc_start(6, sc.SC_NS)
assert owner.blocking_done
assert (owner.plain_start_ns, owner.plain_done_ns) == (0, 2)
assert (owner.tagged_start_ns, owner.tagged_done_ns) == (2, 5)
assert owner.callbacks == 10
events = [str(event) for event in owner.events]
for event in events:
    print("B_SOCKET_EVENT " + event)
assert len(events) == 12
del payload, plain, tagged, owner
gc.collect()
assert int(module_type.destroyed) == 1
print("B_SOCKET_LIFETIME 1")
print("TLM_UTILS_PASSTHROUGH_B_PUBLIC_OK")
