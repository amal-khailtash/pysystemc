"""Verify native scalar signal references, signed values and callback recovery."""

from amal.eda.systemc import sc_core as sc
import cppyy


class Owner:
    """Store Python scalar state and a borrowed native event."""

    def __init__(self, value):
        """Initialize state before native binding."""
        self.value = value
        self.changed = sc.sc_event()

    def read(self):
        """Return scalar state for native conversion."""
        return self.value

    def event(self):
        """Query the native delta notification state."""
        return self.changed.triggered()

    def value_changed_event(self):
        """Borrow the actual notification event."""
        return self.changed

    def write(self, value):
        """Assign a converted scalar and notify next delta."""
        self.value = value
        self.changed.notify(sc.SC_ZERO_TIME)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
ports = []
for value_type, initial, updated in (
    (int, -17, -29),
    (float, 1.25, 2.5),
    ("float", 1.25, 2.5),
    ("long double", 1.25, 2.5),
    ("unsigned long long", 2**63 + 17, 2**63 + 29),
):
    owner = Owner(initial)
    port = sc.sc_port[sc.sc_signal_inout_if[value_type]]()
    port(owner)
    ports.append((owner, port, initial, updated))
del name
seen = []
for owner, port, initial, updated in ports:
    native = port.get_interface()
    assert native.read() == initial and native.get_data_ref() == initial
    assert cppyy.addressof(native.value_changed_event()) == cppyy.addressof(owner.changed)
    top.sc_method(sc.sc_gen_unique_name("observe", False), lambda native=native: seen.append(native.read()))
    top.sensitive(native.default_event())
    top.dont_initialize()
    native.write(updated)
sc.sc_start(1, sc.SC_NS)
assert len(seen) == 5 and sorted(seen) == sorted([-29, 2.5, 2.5, 2.5, 2**63 + 29]), seen
for owner, port, initial, updated in ports:
    native = port.get_interface()
    owner.value = object()
    try:
        native.read()
    except TypeError, ValueError:
        pass
    else:
        raise AssertionError("invalid scalar accepted")
    owner.value = updated
    assert native.read() == updated and native.get_data_ref() == updated
for owner, port, initial, updated in ports:
    if isinstance(updated, float):
        native = port.get_interface()
        assert native.read() == updated
        for invalid in (True, "1.25", object()):
            owner.value = invalid
            for method in (port.get_interface().read, port.get_interface().get_data_ref):
                try:
                    method()
                except TypeError:
                    pass
                else:
                    raise AssertionError("invalid floating result accepted")
            assert native.sampled == updated
            owner.value = updated
            assert native.read() == updated
sc.sc_stop()
print("NATIVE_SCALAR_SIGNAL_OK")
