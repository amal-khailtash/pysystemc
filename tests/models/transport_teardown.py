"""Verify B3 master/slave and transport owners through end hooks and deletion."""

from amal.eda.systemc import sc_core as sc, tlm
import cppyy
import gc
import weakref


class Owner:
    """Provide complete compound and transport callbacks without retaining arguments."""

    def __init__(self):
        """Initialize exact callback counts."""
        self.calls = []

    def put(self, value):
        """Observe a copied compound input."""
        assert value == 17
        self.calls.append("put")

    def get(self):
        """Return a compound output sample."""
        self.calls.append("get")
        return 23

    def peek(self):
        """Return a compound sample without changing state."""
        self.calls.append("peek")
        return 23

    def nb_put(self, value):
        """Accept a copied nonblocking compound input."""
        self.put(value)
        return True

    def nb_get(self):
        """Return a successful nonblocking get."""
        return True, 23

    def nb_peek(self):
        """Return a successful nonblocking peek."""
        return True, 23

    def nb_can_put(self):
        """Report input readiness."""
        return True

    def nb_can_get(self):
        """Report output readiness."""
        return True

    def nb_can_peek(self):
        """Report peek readiness."""
        return True

    def transport(self, value):
        """Return a TLM1 response to copied input."""
        assert value == 17
        self.calls.append("transport")
        return 23

    def nb_transport_fw(self, payload, phase, delay):
        """Complete a borrowed forward transaction."""
        self.calls.append("fw")
        return tlm.TLM_COMPLETED

    def nb_transport_bw(self, payload, phase, delay):
        """Complete a borrowed backward transaction."""
        self.calls.append("bw")
        return tlm.TLM_COMPLETED

    def b_transport(self, payload, delay):
        """Set the response on borrowed payload storage."""
        self.calls.append("blocking")
        payload.set_response_status(tlm.TLM_OK_RESPONSE)

    def transport_dbg(self, payload):
        """Return a deterministic debug byte count."""
        self.calls.append("debug")
        return 7

    def get_direct_mem_ptr(self, payload, dmi):
        """Reject borrowed DMI without waiting."""
        self.calls.append("dmi")
        return False

    def invalidate_direct_mem_ptr(self, start, end):
        """Observe a native invalidation range."""
        assert (start, end) == (17, 23)
        self.calls.append("invalidate")


def exercise(label, native):
    """Invoke native callbacks and verify outputs before teardown."""
    if label in ("master", "slave"):
        native.put(17)
        assert native.get() == 23 and native.peek() == 23
        assert native.nb_can_put() and native.nb_can_get() and native.nb_can_peek()
    elif label == "transport":
        assert native.transport(17) == 23
    else:
        payload = tlm.tlm_generic_payload()
        phase = tlm.tlm_phase(tlm.BEGIN_REQ)
        delay = sc.sc_time(0, sc.SC_NS)
        if label == "fw":
            assert native.nb_transport_fw(payload, phase, delay) == tlm.TLM_COMPLETED
            native.b_transport(payload, delay)
            assert payload.is_response_ok()
            assert native.transport_dbg(payload) == 7
            assert not native.get_direct_mem_ptr(payload, tlm.tlm_dmi())
        else:
            assert native.nb_transport_bw(payload, phase, delay) == tlm.TLM_COMPLETED
            native.invalidate_direct_mem_ptr(17, 23)


class Top(sc.sc_module):
    """Verify retained owners are callable throughout native end hooks."""

    def end_of_simulation(self):
        """Read every compound/transport view before callback or native release."""
        for label, port, adapter_name, reference in groups:
            assert reference() is not None
            exercise(label, port.get_interface())
        ordering.append(("end", str(sc.sc_time_stamp())))


ordering = []
name = sc.sc_module_name("top")
top = Top(name)
groups = []
for label, interface in (
    ("master", tlm.tlm_master_if[int, int]),
    ("slave", tlm.tlm_slave_if[int, int]),
    ("transport", tlm.tlm_transport_if[int, int]),
    ("fw", tlm.tlm_fw_transport_if[tlm.tlm_base_protocol_types]),
    ("bw", tlm.tlm_bw_transport_if[tlm.tlm_base_protocol_types]),
):
    owner = Owner()
    port = sc.sc_port[interface]()
    port(owner)
    groups.append((label, port, port.get_interface().name(), weakref.ref(owner)))
    exercise(label, port.get_interface())
del owner, port, name
gc.collect()
assert all(reference() is not None for label, port, adapter_name, reference in groups)
sc.sc_start(2, sc.SC_NS)
sc.sc_stop()
assert ordering == [("end", "2 ns")], ordering
for label, port, adapter_name, reference in groups:
    exercise(label, port.get_interface())
    port.__destruct__()
assert sc.teardown_interfaces() == 5
ordering.append(("deleted", str(sc.sc_time_stamp())))
gc.collect()
assert all(reference() is None for label, port, adapter_name, reference in groups)
assert all(not cppyy.gbl.sc_core.sc_find_object(adapter_name) for label, port, adapter_name, reference in groups)
assert ordering == [("end", "2 ns"), ("deleted", "2 ns")]
assert sc.teardown_interfaces() == 0
print("TRANSPORT_TEARDOWN_OK 5 2 ns")
