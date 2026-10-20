"""Verify explicit stopped-kernel callback cleanup without deleting borrowed adapters."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import gc
import weakref
import cppyy


class Owner:
    """Provide scalar signal callbacks with a stable native event."""

    def __init__(self):
        """Retain event storage until explicit callback cleanup."""
        self.changed = sc.sc_event()

    def read(self):
        """Return a deterministic scalar sample."""
        return 17

    def event(self):
        """Report no value notification."""
        return False

    def value_changed_event(self):
        """Borrow the retained event."""
        return self.changed


class WideOwner(Owner):
    """Retain a wide native result independently through callback cleanup."""

    def __init__(self):
        """Allocate wide storage and notification event."""
        super().__init__()
        self.value = sc_dt.sc_biguint[137](29)

    def read(self):
        """Borrow a stable wide result."""
        return self.value

    def get_data_ref(self):
        """Borrow the same wide tracing result."""
        return self.value


class TlmOwner:
    """Provide a nonblocking analysis callback for cleanup validation."""

    def __init__(self):
        """Record values observed before release."""
        self.values = []

    def write(self, value):
        """Record one native analysis value."""
        self.values.append(value)


class QueueOwner:
    """Provide combined get/peek callbacks retained through several views."""

    def get(self):
        """Return one scalar sample without waiting."""
        return 43

    def peek(self):
        """Return the same sample without consuming it."""
        return 43

    def nb_get(self):
        """Return a successful nonblocking sample."""
        return True, 43

    def nb_peek(self):
        """Return a successful nonblocking peek."""
        return True, 43

    def nb_can_get(self):
        """Report a sample available."""
        return True

    def nb_can_peek(self):
        """Report a sample available for peeking."""
        return True


class PartialQueueOwner:
    """Expose one operation while retaining self-bound callbacks until release."""

    def __init__(self, operation):
        """Install blocking, nonblocking, and readiness callbacks for one family."""
        setattr(self, operation, self.sample)
        setattr(self, "nb_" + operation, lambda: (True, 47))
        setattr(self, "nb_can_" + operation, lambda: True)

    def sample(self):
        """Return a deterministic value without advancing the stopped kernel."""
        return 47


class Top(sc.sc_module):
    """Verify callback owners remain usable through native lifecycle hooks."""

    def end_of_simulation(self):
        """Read both interfaces before explicit callback cleanup."""
        assert port.get_interface().read() == 17
        assert wide_port.get_interface().read().to_uint64() == 29
        assert queue_ports[1].get_interface().get() == 43
        assert queue_ports[-1].get_interface().peek() == 43
        for partial_port, operation in partial_ports:
            assert getattr(partial_port.get_interface(), operation)() == 47
        ordering.append("end")


ordering = []
name = sc.sc_module_name("top")
top = Top(name)
owner = Owner()
reference = weakref.ref(owner)
port = sc.sc_port[sc.sc_signal_in_if[int]]()
port(owner)
wide_owner = WideOwner()
wide_reference = weakref.ref(wide_owner)
result_reference = weakref.ref(wide_owner.value)
wide_port = sc.sc_port[sc.sc_signal_in_if[sc_dt.sc_biguint[137]]]()
wide_port(wide_owner)
assert wide_port.get_interface().read().to_uint64() == 29
# Ensure the adapter reference cache independently retains returned storage.
wide_owner.value = sc_dt.sc_biguint[137](29)
gc.collect()
assert result_reference() is not None
tlm_owner = TlmOwner()
tlm_reference = weakref.ref(tlm_owner)
tlm_port = sc.sc_port[tlm.tlm_analysis_if[int]]()
tlm_port(tlm_owner)
tlm_port.get_interface().write(31)
assert tlm_owner.values == [31]
queue_owner = QueueOwner()
queue_reference = weakref.ref(queue_owner)
queue_ports = []
for contract in (tlm.tlm_nonblocking_peek_if, tlm.tlm_blocking_get_if, tlm.tlm_get_peek_if):
    queue_port = sc.sc_port[contract[int]]()
    queue_port(queue_owner)
    queue_ports.append(queue_port)
assert queue_ports[-1].get_interface().peek() == 43
partial_ports = []
partial_references = []
for operation in ("get", "peek"):
    partial_owner = PartialQueueOwner(operation)
    partial_references.append(weakref.ref(partial_owner))
    partial_port = sc.sc_port[getattr(tlm, "tlm_" + operation + "_if")[int]]()
    partial_port(partial_owner)
    partial_ports.append((partial_port, operation))
del partial_owner
gc.collect()
assert all(reference() is not None for reference in partial_references)
del queue_owner
del name, owner, wide_owner, tlm_owner
gc.collect()
assert reference() is not None
assert queue_reference() is not None
assert queue_ports[1].get_interface().get() == 43
assert port.get_interface().read() == 17
try:
    sc.release_interface_callbacks()
except RuntimeError:
    pass
else:
    raise AssertionError("cleanup allowed before stop")


def check_running_cleanup():
    """Reject callback release from a running native process."""
    try:
        sc.release_interface_callbacks()
    except RuntimeError:
        ordering.append("running-rejected")
    else:
        raise AssertionError("cleanup allowed during simulation")
    assert queue_ports[-1].get_interface().peek() == 43


top.sc_thread("check_running_cleanup", check_running_cleanup)
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
assert ordering == ["running-rejected", "end"]
assert sc.release_interface_callbacks() == 6
assert sc.release_interface_callbacks() == 0
gc.collect()
assert reference() is None
assert wide_reference() is None
assert result_reference() is None
assert tlm_reference() is None
assert queue_reference() is None
assert all(reference() is None for reference in partial_references)
cppyy.cppdef("""
/** Verify a released shared adapter rejects nonblocking callbacks.
 * Interface and output are borrowed for this call. Exceptions are inspected
 * with the GIL held while clearing Python error state; no references survive.
 */
bool released_peek_output(tlm::tlm_nonblocking_peek_if<int>& channel) {
  int value = 71;
  try { channel.nb_peek(value); }
  catch (...) {
    auto gil = PyGILState_Ensure();
    PyErr_Clear();
    PyGILState_Release(gil);
    return value == 71;
  }
  return false;
}
""")
assert cppyy.gbl.released_peek_output(queue_ports[0].get_interface())
assert cppyy.gbl.released_peek_output(partial_ports[1][0].get_interface())
for partial_port, operation in partial_ports:
    try:
        getattr(partial_port.get_interface(), operation)()
    except (RuntimeError, TypeError) as error:
        assert "released" in str(error)
    else:
        raise AssertionError("partial callback executed after release")
for queue_port, method in ((queue_ports[1], "get"), (queue_ports[-1], "peek")):
    try:
        getattr(queue_port.get_interface(), method)()
    except RuntimeError as error:
        assert "released" in str(error)
    else:
        raise AssertionError("shared TLM callback executed after release")
try:
    port.get_interface().read()
except RuntimeError as error:
    assert "released" in str(error)
else:
    raise AssertionError("callback executed after release")
try:
    tlm_port.get_interface().write(37)
except RuntimeError as error:
    assert "released" in str(error)
else:
    raise AssertionError("TLM callback executed after release")
print("STRUCTURAL_RELEASE_OK")
