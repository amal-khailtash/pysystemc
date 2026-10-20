"""Compare structural scalar signal callbacks with native scalar signals."""

import math

from amal.eda.systemc import sc_core as sc
import cppyy


def integer_token(value):
    """Return an exact decimal token for an integral native sample.

    Args:
        value: Integral value copied from a native signal interface.

    Returns:
        The exact base-ten spelling of the value.
    """
    return str(int(value))


def float_property(value):
    """Classify a float without relying on platform-specific formatting.

    Args:
        value: A Python float copied from a native signal interface.

    Returns:
        One of the normalized finite, zero, infinity or NaN properties.
    """
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "-inf" if value < 0 else "inf"
    if value == 0.0:
        return "-zero" if math.copysign(1.0, value) < 0 else "+zero"
    return "finite-" if value < 0 else "finite+"


class Owner:
    """Implement scalar signal callbacks with stable value and event storage."""

    def __init__(self, initial, *, edged=False):
        """Create event storage and the initial scalar callback value.

        Args:
            initial: Value returned by read callbacks until a write succeeds.
            edged: Whether this bool owner exposes native edge callbacks.
        """
        self.value = initial
        self.changed = sc.sc_event()
        self.rising = sc.sc_event()
        self.falling = sc.sc_event()
        self.edged = edged

    def read(self):
        """Return the current scalar value for native read callbacks.

        Returns:
            The current value, converted by the native interface adapter.
        """
        return self.value

    def get_data_ref(self):
        """Return the current scalar value for native trace-storage callbacks.

        Returns:
            The current value in adapter-owned stable native scalar storage.
        """
        return self.value

    def value_changed_event(self):
        """Return stable native event storage borrowed by signal ports.

        Returns:
            The event retained for the lifetime of this structural owner.
        """
        return self.changed

    def event(self):
        """Report whether the value event fired in the current delta cycle.

        Returns:
            True while the retained value-change event is triggered.
        """
        return self.changed.triggered()

    def posedge_event(self):
        """Return stable rising-edge storage for bool signal ports.

        Returns:
            The rising event retained by this owner.
        """
        return self.rising

    def negedge_event(self):
        """Return stable falling-edge storage for bool signal ports.

        Returns:
            The falling event retained by this owner.
        """
        return self.falling

    def posedge(self):
        """Report whether the rising-edge event fired in the current delta.

        Returns:
            True while the retained rising event is triggered.
        """
        return self.rising.triggered()

    def negedge(self):
        """Report whether the falling-edge event fired in the current delta.

        Returns:
            True while the retained falling event is triggered.
        """
        return self.falling.triggered()

    def write(self, value):
        """Apply native equality and schedule value/edge events on changes.

        Args:
            value: Value converted to the signal's scalar C++ type.
        """
        previous = self.value
        if previous == value:
            return
        self.value = value
        self.changed.notify(sc.SC_ZERO_TIME)
        if self.edged:
            (self.rising if value else self.falling).notify(sc.SC_ZERO_TIME)

    def get_writer_policy(self):
        """Return the native default writer policy.

        Returns:
            The SystemC one-writer policy.
        """
        return sc.SC_ONE_WRITER


def integer_bounds(native_type):
    """Return exact endpoints for a fundamental integer C++ type.

    Args:
        native_type: Fundamental C++ type name used by a signal template.

    Returns:
        Inclusive lower and upper values accepted by the native type.
    """
    if native_type == "bool":
        return 0, 1
    bits = cppyy.sizeof(native_type) * 8
    if native_type.startswith("unsigned"):
        return 0, 2**bits - 1
    return -(2 ** (bits - 1)), 2 ** (bits - 1) - 1


def reject_invalid_reads(owner, interface, invalid, valid_value):
    """Reject malformed scalar results, preserve samples and recover afterward.

    Args:
        owner: Structural callback owner whose result is changed for each case.
        interface: Native signal adapter exposing read and get_data_ref.
        invalid: Iterable of invalid Python callback results.
        valid_value: Known-good sample restored after every rejected read.
    """
    assert interface.read() == valid_value
    for value in invalid:
        owner.value = value
        try:
            interface.read()
        except TypeError:
            pass
        else:
            raise AssertionError(f"invalid signal callback result accepted: {value!r}")
        assert interface.sampled == valid_value
    owner.value = valid_value
    assert interface.read() == valid_value
    assert interface.get_data_ref() == valid_value


name = sc.sc_module_name("top")
top = sc.sc_module(name)
ports = []
integer_types = (
    ("bool", bool),
    ("int", int),
    ("unsigned_int", "unsigned int"),
    ("long_long", "long long"),
    ("unsigned_long_long", "unsigned long long"),
)

# Exercise invalid callback results independently from reference records.
for label, native_type in integer_types:
    type_name = "bool" if native_type is bool else "int" if native_type is int else native_type
    low, high = integer_bounds(type_name)
    initial = bool(low) if native_type is bool else low
    owner = Owner(initial, edged=native_type is bool)
    port = sc.sc_port[sc.sc_signal_inout_if[native_type]](f"error_{label}")
    port(owner)
    interface = port.get_interface()
    if native_type is bool:
        invalid = (0, 1, object())
        assert cppyy.addressof(interface.posedge_event()) == cppyy.addressof(owner.rising)
        assert cppyy.addressof(interface.negedge_event()) == cppyy.addressof(owner.falling)
    else:
        invalid = (True, 1.5, object(), low - 1, high + 1)
    reject_invalid_reads(owner, interface, invalid, initial)

for label, native_type in (("float", "float"), ("double", "double"), ("long_double", "long double")):
    owner = Owner(0.0)
    port = sc.sc_port[sc.sc_signal_inout_if[native_type]](f"error_{label}")
    port(owner)
    reject_invalid_reads(owner, port.get_interface(), (True, object()), 0.0)
print("SIGNAL_CALLBACK_ERRORS_OK")

# Build independent structural owners whose clean records match the native probe.
for label, native_type in integer_types:
    type_name = "bool" if native_type is bool else "int" if native_type is int else native_type
    low, high = integer_bounds(type_name)
    initial, update = (bool(low), bool(high)) if native_type is bool else (low, high)
    owner = Owner(initial, edged=native_type is bool)
    port = sc.sc_port[sc.sc_signal_inout_if[native_type]](label)
    port(owner)
    interface = port.get_interface()
    samples = []
    top.sc_method(
        f"observe_{label}", lambda native=interface, values=samples: values.append((native.read(), native.event()))
    )
    top.sensitive(interface.default_event())
    top.dont_initialize()
    ports.append((label, owner, interface, initial, update, samples, False))


for label, native_type in (("float", "float"), ("double", "double"), ("long_double", "long double")):
    owner = Owner(0.0)
    port = sc.sc_port[sc.sc_signal_inout_if[native_type]](label)
    port(owner)
    interface = port.get_interface()
    negative_zero_owner = Owner(-0.0)
    negative_zero_port = sc.sc_port[sc.sc_signal_inout_if[native_type]](f"{label}_negative_zero")
    negative_zero_port(negative_zero_owner)
    negative_zero = negative_zero_port.get_interface()
    assert math.copysign(1.0, negative_zero.read()) < 0
    samples = []
    top.sc_method(
        f"observe_{label}", lambda native=interface, values=samples: values.append((native.read(), native.event()))
    )
    top.sensitive(interface.default_event())
    top.dont_initialize()
    ports.append((label, owner, interface, negative_zero, None, samples, True))


for label, owner, interface, initial, update, samples, floating in ports:
    if not floating:
        interface.write(update)
for label, owner, interface, negative_zero, unused, samples, floating in ports:
    if floating:
        interface.write(-0.0)
del name
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_start(sc.SC_ZERO_TIME)
for label, owner, interface, negative_zero, unused, samples, floating in ports:
    if floating:
        assert not samples and math.copysign(1.0, interface.read()) > 0
for label, owner, interface, initial, update, samples, floating in ports:
    if not floating:
        interface.write(initial)
for label, owner, interface, negative_zero, unused, samples, floating in ports:
    if floating:
        interface.write(float("inf"))
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_start(sc.SC_ZERO_TIME)
for label, owner, interface, negative_zero, unused, samples, floating in ports:
    if floating:
        interface.write(float("nan"))
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_start(sc.SC_ZERO_TIME)
for label, owner, interface, negative_zero, unused, samples, floating in ports:
    if floating:
        interface.write(1.25)
sc.sc_start(sc.SC_ZERO_TIME)
sc.sc_start(sc.SC_ZERO_TIME)

for label, owner, interface, initial, update, samples, floating in ports:
    if not floating:
        assert len(samples) == 2, (label, samples)
        assert all(event for value, event in samples), (label, samples)
        values = ",".join(integer_token(value) for value, event in samples)
        print(f"SIGNAL_MATRIX {label}|{len(samples)}|{values}|{integer_token(interface.read())}|{sc.sc_time_stamp()}")
    else:
        assert len(samples) == 3, (label, samples)
        assert all(event for value, event in samples), (label, samples)
        properties = ",".join(float_property(value) for value, event in samples)
        assert interface.read() == 1.25
        print(
            f"SIGNAL_FLOAT {label}|{properties}|{float_property(interface.read())}|"
            f"{float_property(0.0)}|{float_property(initial.read())}|{sc.sc_time_stamp()}"
        )
sc.sc_stop()
print("SIGNAL_TYPE_MATRIX_OK")
