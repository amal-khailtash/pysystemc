"""Compare structural floating signal traces with native signal references."""

from amal.eda.systemc import sc_core as sc


class Owner:
    """Retain floating storage and the event borrowed by a signal input view."""

    def __init__(self):
        """Initialize the callback value and native event."""
        self.value = 1.25
        self.changed = sc.sc_event()

    def read(self):
        """Return the current floating sample for stable adapter storage."""
        return self.value

    def event(self):
        """Report the current native event state."""
        return self.changed.triggered()

    def value_changed_event(self):
        """Borrow the retained native event."""
        return self.changed


name = sc.sc_module_name("top")
top = sc.sc_module(name)
trace = sc.sc_create_vcd_trace_file("structural_float")
trace.set_time_unit(1, sc.SC_NS)
channels = []
for datatype in ("float", "double"):
    owner = Owner()
    port = sc.sc_port[sc.sc_signal_in_if[datatype]]()
    port(owner)
    sc.sc_trace(trace, port.get_interface(), datatype.replace(" ", "_"))
    channels.append((owner, port))
unsupported_owner = Owner()
unsupported_port = sc.sc_port[sc.sc_signal_in_if["long double"]]()
unsupported_port(unsupported_owner)
try:
    sc.sc_trace(trace, unsupported_port.get_interface(), "long_double")
except TypeError as error:
    assert "long double overload" in str(error)
else:
    raise AssertionError("unsupported native long double trace accepted")
del name
sc.sc_start(1, sc.SC_NS)
for owner, port in channels:
    owner.value = -2.5
    owner.changed.notify(sc.SC_ZERO_TIME)
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
sc.sc_close_vcd_trace_file(trace)
print("STRUCTURAL_FLOAT_TRACE_OK")
