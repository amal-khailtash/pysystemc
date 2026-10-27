"""Exercise public native multi-passthrough socket APIs through cppyy."""

import gc
from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils

import cppyy


PROBE = Path(__file__).resolve().parents[1] / "native/tlm_utils_multi_probe.h"
cppyy.include(str(PROBE))
probe = cppyy.gbl.pysystemc_tlm_utils_multi
owner_type = probe.Owner
policy = sc.SC_ZERO_OR_MORE_BOUND
bind_mode = sys.argv[1] if len(sys.argv) > 1 else "bind"
assert bind_mode in {"bind", "call"}
initiator_type = tlm_utils.multi_passthrough_initiator_socket[owner_type, 73, probe.Protocol, 0, policy]
target_type = tlm_utils.multi_passthrough_target_socket[owner_type, 73, probe.Protocol, 0, policy]


def member(owner_name, method_name):
    """Return a fully qualified native callback member spelling."""
    return f"{owner_name}::{method_name}"


def as_text(value):
    """Convert a native value to the stable decimal/string form used by records."""
    return str(value.to_string())


assert initiator_type.transaction_type.__cpp_name__.endswith("::Payload")
assert "tlm_phase" in initiator_type.phase_type.__cpp_name__
assert initiator_type.sync_enum_type is int
assert "multi_init_base" in initiator_type.base_type.__cpp_name__
assert "tlm_base_target_socket_b" in initiator_type.base_target_socket_type.__cpp_name__
assert target_type.transaction_type is initiator_type.transaction_type
assert target_type.phase_type is initiator_type.phase_type
assert target_type.sync_enum_type is initiator_type.sync_enum_type
assert "multi_target_base" in target_type.base_type.__cpp_name__
assert "tlm_base_initiator_socket_b" in target_type.base_initiator_socket_type.__cpp_name__
assert str(initiator_type.default_name()).startswith("multi_passthrough_initiator_socket")
assert str(target_type.default_name()).startswith("multi_passthrough_target_socket")

# Null holders, copied values, and failed exact-signature selection are native.
null_nb = initiator_type.nb_cb()
assert not null_nb.valid()
assert not initiator_type.nb_cb(null_nb).valid()
assert not initiator_type.dmi_cb().valid()
try:
    initiator_type.nb_cb.from_member(member("pysystemc_tlm_utils_multi::Owner", "target_b"))
except TypeError:
    pass
else:
    raise AssertionError("incompatible initiator callback signature was accepted")

probe.SocketLifecycle.reset()
owner = owner_type(sc.sc_module_name("top"))
assert isinstance(owner.outer_initiator, type(owner.inner_initiator))
assert isinstance(owner.outer_target, type(owner.inner_target))
assert owner.outer_initiator.kind() == "tlm_initiator_socket"
assert owner.outer_target.kind() == "tlm_target_socket"

# Register all six exact native callback typedefs through their value holders.
owner.inner_target.check_export_binding()
initiator_nb = initiator_type.nb_cb.from_member(member("pysystemc_tlm_utils_multi::Owner", "initiator_nb"))
initiator_dmi = initiator_type.dmi_cb.from_member(member("pysystemc_tlm_utils_multi::Owner", "initiator_invalidate"))
target_callbacks = (
    ("nb_cb", "register_nb_transport_fw", "target_nb"),
    ("b_cb", "register_b_transport", "target_b"),
    ("dbg_cb", "register_transport_dbg", "target_dbg"),
    ("dmi_cb", "register_get_direct_mem_ptr", "target_dmi"),
)
assert initiator_nb.valid() and initiator_dmi.valid()
initiator_nb = initiator_type.nb_cb(initiator_nb)
initiator_dmi = initiator_type.dmi_cb(initiator_dmi)
assert initiator_nb.valid() and initiator_dmi.valid()
initiator_type.register_nb_transport_bw(owner.outer_initiator, owner, initiator_nb)
initiator_type.register_invalidate_direct_mem_ptr(owner.outer_initiator, owner, initiator_dmi)
for registration, alias in (
    ("register_nb_transport_bw", initiator_type.nb_cb),
    ("register_invalidate_direct_mem_ptr", initiator_type.dmi_cb),
):
    null_callback = alias()
    try:
        null_callback.register_on(owner.outer_initiator, owner)
    except cppyy.gbl.std.invalid_argument:
        pass
    else:
        raise AssertionError("native registration accepted a null initiator callback")
    try:
        getattr(initiator_type, registration)(owner.outer_initiator, owner, null_callback)
    except TypeError:
        pass
    else:
        raise AssertionError("Python registration accepted a null initiator callback")
for alias, registration, callback in target_callbacks:
    alias_type = getattr(target_type, alias)
    assert not alias_type().valid()
    callback_value = getattr(target_type, alias).from_member(member("pysystemc_tlm_utils_multi::Owner", callback))
    assert callback_value.valid()
    callback_value = alias_type(callback_value)
    assert callback_value.valid()
    assert getattr(target_type, registration)(owner.inner_target, owner, callback_value) is None
    null_callback = alias_type()
    try:
        null_callback.register_on(owner.inner_target, owner)
    except cppyy.gbl.std.invalid_argument:
        pass
    else:
        raise AssertionError("native registration accepted a null target callback")
    try:
        getattr(target_type, registration)(owner.inner_target, owner, null_callback)
    except TypeError:
        pass
    else:
        raise AssertionError("Python registration accepted a null target callback")

# Exercise both hierarchy bind overloads in separate fresh-kernel executions.
if bind_mode == "bind":
    owner.outer_initiator.bind(owner.inner_initiator)
    owner.outer_target.bind(owner.inner_target)
else:
    owner.outer_initiator(owner.inner_initiator)
    owner.outer_target(owner.inner_target)
owner.inner_initiator.bind(owner.forward_target_0)
owner.inner_initiator(owner.forward_target_1)
owner.backward_initiator_0.bind(owner.outer_target)
owner.backward_initiator_1.bind(owner.outer_target)
sc.sc_start(sc.SC_ZERO_TIME)
assert [owner.outer_initiator.size(), owner.inner_initiator.size()] == [2, 2]
assert [owner.outer_target.size(), owner.inner_target.size()] == [2, 2]
assert probe.SocketLifecycle.initiator_before_end()[0] == 2
assert probe.SocketLifecycle.target_end()[0] == 2

# Base accessors return live native references. Const interface access is denied
# by the upstream multi sockets and the helper confirms the expected report.
initiator_interface = owner.outer_initiator.get_base_interface()
target_interface = owner.outer_target.get_base_interface()
initiator_export = owner.inner_initiator.get_base_export()
target_export = owner.inner_target.get_base_export()
assert cppyy.addressof(initiator_interface) != 0
assert cppyy.addressof(target_interface) != 0
assert cppyy.addressof(initiator_export) != 0
assert cppyy.addressof(target_export) != 0
assert probe.const_initiator_interface_rejected(owner.outer_initiator)
assert probe.const_target_interface_rejected(owner.outer_target)
assert probe.const_initiator_export(owner.outer_initiator) != 0
assert probe.const_target_export(owner.outer_target) != 0

records = []
forward_targets = (owner.forward_target_0, owner.forward_target_1)
for index in range(2):
    payload = probe.Payload()
    payload.set_address(0xA000 + index)
    payload.value = probe.wide(index)
    delay = sc.sc_time(0, sc.SC_NS)
    owner.outer_initiator[index].b_transport(payload, delay)
    assert payload.get_address() == 0xA100 + index
    assert as_text(payload.value) == as_text(probe.wide(0x10 + index))
    assert delay == sc.sc_time(10 + index, sc.SC_NS)
    records.append(f"INIT_B {index} {payload.get_address()} {as_text(payload.value)} {delay}")

    payload.set_address(0xA000 + index)
    phase = tlm.tlm_phase(tlm.BEGIN_REQ)
    delay = sc.sc_time(0, sc.SC_NS)
    assert owner.outer_initiator[index].nb_transport_fw(payload, phase, delay) == tlm.TLM_UPDATED
    assert phase == tlm.BEGIN_RESP and delay == sc.sc_time(20 + index, sc.SC_NS)
    assert owner.initiator_nb_calls[index] == 1
    assert owner.last_backward_phase[index] == int(tlm.END_RESP)
    records.append(f"INIT_NB {index} {phase} {delay} {owner.initiator_nb_calls[index]}")

    payload.set_address(0xA000 + index)
    debug_count = owner.outer_initiator[index].transport_dbg(payload)
    assert debug_count == 30 + index
    records.append(f"INIT_DBG {index} {debug_count}")

    payload.set_address(0xA000 + index)
    dmi = tlm.tlm_dmi()
    assert owner.outer_initiator[index].get_direct_mem_ptr(payload, dmi)
    assert (dmi.get_start_address(), dmi.get_end_address()) == (0xE000 + index, 0xE0FF + index)
    assert dmi.is_read_allowed() and dmi.is_write_allowed()
    assert dmi.get_read_latency() == sc.sc_time(1 + index, sc.SC_NS)
    assert dmi.get_write_latency() == sc.sc_time(2 + index, sc.SC_NS)
    records.append(
        f"INIT_DMI {index} {dmi.get_start_address()} {dmi.get_end_address()} {dmi.get_read_latency()} {dmi.get_write_latency()}"
    )

    forward_targets[index][0].invalidate_direct_mem_ptr(0xC000 + index, 0xC0FF + index)
    assert owner.initiator_dmi_calls[index] == 1
    assert owner.last_invalidation[index] == index
    records.append(f"INIT_INVALIDATE {index} {owner.initiator_dmi_calls[index]}")

    payload = probe.Payload()
    payload.set_address(0xB000 + index)
    delay = sc.sc_time(0, sc.SC_NS)
    owner.issue_target_b(index, payload, delay)
    assert payload.get_address() == 0xB100 + index
    assert as_text(payload.value) == as_text(probe.wide(0x40 + index))
    assert delay == sc.sc_time(40 + index, sc.SC_NS)
    records.append(f"TARGET_B {index} {payload.get_address()} {as_text(payload.value)} {delay}")

    payload.set_address(0xB000 + index)
    phase = tlm.tlm_phase(tlm.BEGIN_REQ)
    delay = sc.sc_time(0, sc.SC_NS)
    assert owner.issue_target_nb(index, payload, phase, delay) == tlm.TLM_UPDATED
    assert phase == tlm.BEGIN_RESP and delay == sc.sc_time(50 + index, sc.SC_NS)
    assert owner.backward_nb_calls[index] == 1
    records.append(f"TARGET_NB {index} {phase} {delay} {owner.backward_nb_calls[index]}")

    payload.set_address(0xB000 + index)
    assert owner.issue_target_dbg(index, payload) == 70 + index
    records.append(f"TARGET_DBG {index} {70 + index}")

    payload.set_address(0xB000 + index)
    dmi = tlm.tlm_dmi()
    assert not owner.issue_target_dmi(index, payload, dmi)
    assert (dmi.get_start_address(), dmi.get_end_address()) == (0xD000 + index, 0xD0FF + index)
    assert dmi.is_none_allowed()
    records.append(f"TARGET_DMI {index} {dmi.get_start_address()} {dmi.get_end_address()}")

    payload.set_address(0xB000 + index)
    phase = tlm.tlm_phase(tlm.BEGIN_RESP)
    delay = sc.sc_time(0, sc.SC_NS)
    assert owner.inner_target[index].nb_transport_bw(payload, phase, delay) == tlm.TLM_COMPLETED
    assert phase == tlm.END_RESP and delay == sc.sc_time(80 + index, sc.SC_NS)
    owner.inner_target[index].invalidate_direct_mem_ptr(0xC000 + index, 0xC0FF + index)
    assert owner.backward_dmi_calls[index] == 1
    records.append(f"TARGET_BW {index} {phase} {delay} {owner.backward_dmi_calls[index]}")

for record in records:
    print("MULTI_SOCKET_EVENT " + record)
assert [owner.call_total(i) for i in range(2)] == [13, 13]
assert len(records) == 20
sc.sc_start(sc.sc_time(3, sc.SC_NS))
assert sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
print(f"MULTI_SOCKET_TIME {sc.sc_time_stamp()}")

# Keep destruction observable after every borrowed reference is released.
del payload, dmi, initiator_interface, target_interface, initiator_export, target_export
del forward_targets
owner = None
gc.collect()
assert list(probe.SocketLifecycle.initiator_destroyed()) == [2, 0]
assert list(probe.SocketLifecycle.target_destroyed()) == [2, 0]
print("MULTI_SOCKET_LIFETIME 2 2")
print("TLM_UTILS_MULTI_PUBLIC_OK")
