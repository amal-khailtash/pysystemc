"""Audit every registered structural adapter family through native destruction."""

import gc
import weakref
from typing import Protocol
from amal.eda.systemc import sc_core as sc, sc_dt, tlm
from amal.eda.systemc.sc_core import _interfaces
import cppyy


class State:
    """Retain callback storage independently for each native adapter family."""

    def __init__(self):
        """Allocate a native event and mutable wide reference result."""
        self.changed = sc.sc_event()
        self.value = sc_dt.sc_biguint[137](17)
        self.calls = []


def callback(method):
    """Build a callback with the registered native result contract."""

    def invoke(self, *args):
        """Record callback execution and return valid native scalar/reference state."""
        self.calls.append(method)
        if method.endswith("event"):
            return self.changed
        if method == "get_writer_policy":
            return sc.SC_ONE_WRITER
        if method in ("read", "get", "peek", "get_data_ref"):
            return 17
        if method in ("nb_read", "nb_get", "nb_peek"):
            return True, 17
        if method.startswith("nb_can_") or method in ("nb_write", "nb_put", "nb_poke", "nb_reduce", "nb_bound"):
            return True
        if method in ("event", "posedge", "negedge"):
            return False
        if method in (
            "num_available",
            "num_free",
            "used",
            "size",
            "lock",
            "trylock",
            "unlock",
            "wait",
            "trywait",
            "post",
            "get_value",
            "transport_dbg",
        ):
            return 0
        if method == "transport":
            return 23
        if method in ("nb_transport_fw", "nb_transport_bw"):
            return tlm.TLM_COMPLETED
        if method == "get_direct_mem_ptr":
            return False
        return None

    return invoke


name = sc.sc_module_name("top")
top = sc.sc_module(name)
groups = []
# The registry is the finite supported-family inventory, not a type whitelist.
for family, methods in list(_interfaces._METHODS.items()):
    if family.startswith("pysystemc_interfaces::"):
        continue
    namespace, stem = family.split("::")
    template = getattr(getattr(cppyy.gbl, namespace), stem)
    if stem in (
        "sc_mutex_if",
        "sc_semaphore_if",
        "sc_event_queue_if",
        "tlm_bw_direct_mem_if",
        "tlm_fifo_config_size_if",
    ):
        contract = template
    elif "master_if" in stem or "slave_if" in stem or stem == "tlm_transport_if":
        contract = template[int, int]
    elif stem in ("tlm_fw_transport_if", "tlm_bw_transport_if"):
        contract = template[tlm.tlm_base_protocol_types]
    elif stem in ("tlm_fw_nonblocking_transport_if", "tlm_bw_nonblocking_transport_if"):
        contract = template[tlm.tlm_generic_payload, tlm.tlm_phase]
    elif stem in ("tlm_blocking_transport_if", "tlm_fw_direct_mem_if", "tlm_transport_dbg_if"):
        contract = template[tlm.tlm_generic_payload]
    else:
        contract = template[int]
    owner_type = type(
        "Owner" + str(len(groups)), (State,), {method: callback(method) for method in set(methods.values())}
    )
    owner = owner_type()
    port = sc.sc_port[contract]()
    port(owner)
    native = port.get_interface()
    # Invoke a native public method for every concrete adapter shape.
    payload, delay = tlm.tlm_generic_payload(), sc.sc_time(0, sc.SC_NS)
    phase = tlm.tlm_phase(tlm.BEGIN_REQ)
    if "transport_if" in stem or "transport_dbg_if" in stem:
        if stem == "tlm_transport_if":
            assert native.transport(17) == 23
        elif stem in ("tlm_fw_transport_if", "tlm_fw_nonblocking_transport_if"):
            assert native.nb_transport_fw(payload, phase, delay) == tlm.TLM_COMPLETED
        elif stem in ("tlm_bw_transport_if", "tlm_bw_nonblocking_transport_if"):
            assert native.nb_transport_bw(payload, phase, delay) == tlm.TLM_COMPLETED
        elif stem == "tlm_blocking_transport_if":
            native.b_transport(payload, delay)
        else:
            assert native.transport_dbg(payload) == 0
    elif stem == "tlm_fw_direct_mem_if":
        assert not native.get_direct_mem_ptr(payload, tlm.tlm_dmi())
    elif stem == "tlm_bw_direct_mem_if":
        native.invalidate_direct_mem_ptr(0, 1)
    elif stem == "tlm_fifo_config_size_if":
        native.nb_expand()
    elif stem == "tlm_fifo_debug_if":
        assert native.used() == 0
    elif stem == "sc_mutex_if":
        assert native.trylock() == 0
    elif stem == "sc_semaphore_if":
        assert native.get_value() == 0
    elif stem == "sc_event_queue_if":
        assert native.default_event() == owner.changed
    elif "signal" in stem:
        if stem == "sc_signal_write_if":
            native.write(17)
        else:
            assert native.read() == 17
    elif "fifo" in stem and "out" in stem:
        if "nonblocking" in stem:
            assert native.nb_write(17)
        else:
            native.write(17)
    elif "fifo" in stem and "in" in stem:
        if "nonblocking" in stem:
            assert native.data_written_event() == owner.changed
        else:
            assert native.read() == 17
    elif stem in ("tlm_analysis_if", "tlm_write_if"):
        native.write(17)
    elif stem in ("tlm_delayed_write_if", "tlm_delayed_analysis_if"):
        native.write(17, sc.sc_time(2, sc.SC_NS))
    elif "put" in stem:
        if "nonblocking" in stem:
            assert native.nb_put(17)
        else:
            native.put(17)
    elif "nonblocking" in stem:
        assert native.nb_can_get() if "get" in stem or "master" in stem or "slave" in stem else native.nb_can_peek()
    elif "get" in stem or "master" in stem or "slave" in stem:
        assert native.get() == 17
    else:
        assert native.peek() == 17
    assert owner.calls, family
    groups.append((family, port, native.name(), weakref.ref(owner), weakref.ref(owner.value)))
# Include the combined FIFO bridge through all compatible native views.
methods = _interfaces._METHODS["pysystemc_interfaces::FifoCombinedInterface"]
owner_type = type("CombinedOwner", (State,), {method: callback(method) for method in set(methods.values())})
owner = owner_type()
port = sc.sc_port[tlm.tlm_fifo_debug_if[int]]()
port(owner)
groups.append(("combined_fifo", port, port.get_interface().name(), weakref.ref(owner), weakref.ref(owner.value)))


@sc.interface(methods={"reference_value": ("const sc_dt::sc_biguint<137>&", "")})
class ReferenceContract(Protocol):
    """Include generated Protocol callbacks with retained native results."""


class ReferenceOwner(State):
    """Supply a reference result retained independently by the adapter."""

    def reference_value(self):
        """Borrow current native storage."""
        return self.value


owner = ReferenceOwner()
port = sc.sc_port[ReferenceContract]()
port(owner)
assert port.get_interface().reference_value().to_uint64() == 17
groups.append(("generated_protocol", port, port.get_interface().name(), weakref.ref(owner), weakref.ref(owner.value)))
del owner, native, port, name
assert len(groups) == len(_interfaces._METHODS)
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
try:
    sc.teardown_interfaces()
except RuntimeError:
    pass
else:
    raise AssertionError("live family borrowers accepted")
assert all(ref() is not None and value() is not None for _, _, _, ref, value in groups)
released = sc.release_interface_callbacks()
assert released == len(_interfaces._owners)
assert sc.release_interface_callbacks() == 0
for family, port, _, _, _ in groups:
    adapter = port.get_interface()
    native_interface = next(entry[0] for entry in _interfaces._owners if entry[2].name() == adapter.name())
    method = next(
        name for name, member in type(adapter).__dict__.items() if getattr(member, "__name__", "") == "forward"
    )
    # Invoke the registered forward method directly. Its first operation is
    # the release guard, before native callback arguments are inspected.
    try:
        type(adapter).__dict__[method](adapter)
    except RuntimeError as error:
        assert "released" in str(error), (family, str(error))
    else:
        raise AssertionError((family, "released callback accepted"))
del adapter
gc.collect()
assert all(ref() is None and value() is None for _, _, _, ref, value in groups), "released callback storage retained"
for _, port, _, _, _ in groups:
    port.__destruct__()
del port
expected = len(_interfaces._owners)
assert sc.teardown_interfaces() == expected
assert sc.teardown_interfaces() == 0
gc.collect()
assert all(ref() is None and value() is None for _, _, _, ref, value in groups), "family callback storage retained"
assert all(not sc.sc_find_object(name) for _, _, name, _, _ in groups), "native family registry entry retained"
print("RETAINED_FAMILY_TEARDOWN_OK", len(groups), expected)
