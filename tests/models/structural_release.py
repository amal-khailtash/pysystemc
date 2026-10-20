"""Verify explicit stopped-kernel callback cleanup without deleting borrowed adapters."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import gc
import weakref


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


class Top(sc.sc_module):
    """Verify callback owners remain usable through native lifecycle hooks."""

    def end_of_simulation(self):
        """Read both interfaces before explicit callback cleanup."""
        assert port.get_interface().read() == 17
        assert wide_port.get_interface().read().to_uint64() == 29
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
del name, owner, wide_owner, tlm_owner
gc.collect()
assert reference() is not None
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


top.sc_thread("check_running_cleanup", check_running_cleanup)
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
assert ordering == ["running-rejected", "end"]
assert sc.release_interface_callbacks() == 3
assert sc.release_interface_callbacks() == 0
gc.collect()
assert reference() is None
assert wide_reference() is None
assert result_reference() is None
assert tlm_reference() is None
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
