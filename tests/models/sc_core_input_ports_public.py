"""Verify generic, bool and logic sc_in declarations through cppyy."""

from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy

PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_core_input_ports_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.pysystemc_input_ports
WIDE = dt.sc_bv[137]


def make_value(label, seed):
    """Create distinct bool, logic or 137-bit values for channels and writes.

    Args:
        label: ``wide``, ``bool`` or ``logic``.
        seed: Selects a deterministic value for this use.

    Returns:
        A value of the matching native SystemC datatype.
    """
    if label == "bool":
        return bool(seed % 2)
    if label == "logic":
        return dt.sc_logic(("0", "1", "X", "Z")[seed % 4])
    bits = ["1" if (index * 11 + seed * 7) % 19 < 8 else "0" for index in range(137)]
    bits[0] = "1" if seed % 2 else "0"
    bits[-1] = "0" if seed % 2 else "1"
    return WIDE("".join(bits))


def value_text(value):
    """Render native values with the same spelling as the C++ reference."""
    if isinstance(value, bool):
        return "1" if value else "0"
    rendered = value.to_string() if hasattr(value, "to_string") else str(value)
    return rendered.decode() if isinstance(rendered, bytes) else str(rendered)


def make_constructor_matrix(label, value_type):
    """Exercise all ten sc_in constructors and retain their borrowed owners.

    Args:
        label: Stable type-family name used to form SystemC object names.
        value_type: bool, sc_logic or sc_bv[137].

    Returns:
        A tuple of the ten ports and objects that provide their bindings.
    """
    port_type = sc.sc_in[value_type]
    signal_type = sc.sc_signal[value_type]
    ports = []
    owners = []
    channels = []
    for index in range(10):
        channel = signal_type(f"{label}_ctor_signal_{index}", make_value(label, 0))
        channels.append(channel)
        owners.append(channel)

    ports.append(port_type())
    ports[0].bind(channels[0])
    ports.append(port_type(f"{label}_ctor_named"))
    ports[1].bind(channels[1])
    ports.append(port_type(channels[2]))
    ports.append(port_type(f"{label}_ctor_named_interface", channels[3]))

    input_port_type = port_type.in_port_type
    for index in (4, 5):
        parent = input_port_type(f"{label}_ctor_parent_{index}")
        parent.bind(channels[index])
        owners.append(parent)
        ports.append(port_type(parent) if index == 4 else port_type(f"{label}_ctor_named_parent", parent))

    inout_port_type = port_type.inout_port_type
    for index in (6, 7):
        parent = inout_port_type(f"{label}_ctor_inout_parent_{index}")
        parent.bind(channels[index])
        owners.append(parent)
        ports.append(port_type(parent) if index == 6 else port_type(f"{label}_ctor_named_inout_parent", parent))

    same_type_parent = port_type(f"{label}_ctor_this_parent")
    same_type_parent.bind(channels[8])
    owners.append(same_type_parent)
    ports.append(port_type(same_type_parent))
    named_same_type_parent = port_type(f"{label}_ctor_named_this_parent")
    named_same_type_parent.bind(channels[9])
    owners.append(named_same_type_parent)
    ports.append(port_type(f"{label}_ctor_named_this", named_same_type_parent))

    assert len(ports) == 10
    assert all(port.bind_count() == 1 and port.kind() == "sc_in" for port in ports)
    return ports, owners


def make_binding_matrix(label, value_type):
    """Exercise interface/input-parent/inout-parent bind and call methods.

    Args:
        label: Stable type-family name used for native object names.
        value_type: bool, sc_logic or sc_bv[137].

    Returns:
        Seven targets and all signal/port objects borrowed by those targets.
    """
    input_type = sc.sc_in[value_type]
    signal_type = sc.sc_signal[value_type]
    channels = [signal_type(f"{label}_bind_channel_{index}", make_value(label, 0)) for index in range(7)]
    targets = [input_type(f"{label}_bind_target_{index}") for index in range(7)]
    input_parent_type = input_type.in_port_type
    inout_parent_type = input_type.inout_port_type
    input_parents = [input_parent_type(f"{label}_input_parent_{index}") for index in range(2)]
    inout_parents = [inout_parent_type(f"{label}_inout_parent_{index}") for index in range(2)]
    input_parents[0].bind(channels[3])
    input_parents[1].bind(channels[4])
    inout_parents[0].bind(channels[5])
    inout_parents[1].bind(channels[6])
    targets[0].bind(channels[0])
    const_interface = probe.const_interface_ref[input_type.in_if_type](channels[1])
    targets[1].bind(const_interface)
    targets[2](channels[2])
    targets[3].bind(input_parents[0])
    targets[4](input_parents[1])
    targets[5].bind(inout_parents[0])
    targets[6](inout_parents[1])
    assert all(target.bind_count() == 1 for target in targets)
    assert all(target.kind() == "sc_in" for target in targets)
    assert [target.size() for target in targets] == [1, 1, 1, 0, 0, 0, 0]
    return targets, [*channels, *input_parents, *inout_parents]


def check_aliases(value_type):
    """Check every public generic/specialized sc_in typedef in cppyy."""
    input_type = sc.sc_in[value_type]
    inout_type = sc.sc_inout[value_type]
    if value_type is bool:
        assert input_type.data_type.__cpp_name__.endswith("::data_type")
    else:
        assert input_type.data_type.__cpp_name__ == value_type.__cpp_name__
    assert input_type.if_type.__cpp_name__ == input_type.in_if_type.__cpp_name__
    assert input_type.this_type.__cpp_name__ == input_type.__cpp_name__
    assert input_type.base_type.__cpp_name__.startswith("sc_core::sc_port<")
    assert input_type.base_port_type.__cpp_name__.startswith("sc_core::sc_port_b<")
    assert input_type.in_port_type.__cpp_name__ == input_type.base_type.__cpp_name__
    assert input_type.inout_port_type.__cpp_name__.startswith("sc_core::sc_port<")
    assert input_type.inout_if_type.__cpp_name__.startswith("sc_core::sc_signal_inout_if<")
    assert inout_type.__cpp_name__ != input_type.__cpp_name__


class Wave:
    """Own a deferred-trace input port and its signal channel."""

    def __init__(self, label, value_type):
        """Construct an unbound input and initial signal for prebinding trace."""
        self.label = label
        self.value_type = value_type
        self.signal = sc.sc_signal[value_type]
        self.channel = self.signal(f"{label}_wave_channel", make_value(label, 0))
        self.port = sc.sc_in[value_type](f"{label}_wave_input")

    def bind(self):
        """Bind the traced input to its retained channel."""
        self.port.bind(self.channel)


class InputModel(sc.sc_module):
    """Own constructor, binding, trace and event behavior in one module."""

    def __init__(self, name):
        """Build all input families and retain event callbacks through simulation."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.types = {"wide": WIDE, "bool": bool, "logic": dt.sc_logic}
        self.constructor_ports = {}
        self.constructor_owners = {}
        self.binding_ports = {}
        self.binding_owners = {}
        self.waves = {}
        self.trace_probes = {}
        self.trace_channels = {}
        self.finder_flags = {}
        self.events = []
        self.phase = 0
        for label, value_type in self.types.items():
            check_aliases(value_type)
            ports, owners = make_constructor_matrix(label, value_type)
            self.constructor_ports[label] = ports
            self.constructor_owners[label] = owners
            binding_ports, binding_owners = make_binding_matrix(label, value_type)
            self.binding_ports[label] = binding_ports
            self.binding_owners[label] = binding_owners
            self.waves[label] = Wave(label, value_type)
            signal = sc.sc_signal[value_type]
            trace_channel = signal(f"{label}_member_trace_channel", make_value(label, 0))
            self.trace_channels[label] = trace_channel
            probe_type = {
                "wide": probe.InputWideProbe,
                "bool": probe.InputBoolProbe,
                "logic": probe.InputLogicProbe,
            }[label]
            trace_probe = probe_type(f"{label}_member_trace_input")
            trace_probe.bind(trace_channel)
            self.trace_probes[label] = trace_probe

        self.virtual_destruction_verified = probe.verify_virtual_destruction()
        self._register_observers()
        self.sc_method("drive", self.drive)
        del module_name

    def _register_observers(self):
        """Cache value/edge finders and attach methods to native sensitivities."""
        wide_port = self.waves["wide"].port
        wide_finder = wide_port.value_changed()
        self.finder_flags["wide"] = (cppyy.addressof(wide_finder) == cppyy.addressof(wide_port.value_changed()),)
        self.sc_method("wide_change", lambda: self.observe("wide", "value"))
        self.sensitive(wide_finder)
        self.dont_initialize()

        for label in ("bool", "logic"):
            port = self.waves[label].port
            changed = port.value_changed()
            positive = port.pos()
            negative = port.neg()
            self.finder_flags[label] = (
                cppyy.addressof(changed) == cppyy.addressof(port.value_changed()),
                cppyy.addressof(positive) == cppyy.addressof(port.pos()),
                cppyy.addressof(negative) == cppyy.addressof(port.neg()),
            )
            self.sc_method(f"{label}_change", lambda label=label: self.observe(label, "value"))
            self.sensitive(changed)
            self.dont_initialize()
            self.sc_method(f"{label}_posedge", lambda label=label: self.observe(label, "pos"))
            self.sensitive(positive)
            self.dont_initialize()
            self.sc_method(f"{label}_negedge", lambda label=label: self.observe(label, "neg"))
            self.sensitive(negative)
            self.dont_initialize()

    def bind_waves(self):
        """Bind deferred trace ports and verify read/conversion reference identity."""
        for wave in self.waves.values():
            wave.bind()
            assert probe.conversion_value_address(wave.port) == probe.read_value_address(wave.port)

    def add_traces(self, trace):
        """Register traced ports and exercise legacy member add_trace records."""
        for label, wave in self.waves.items():
            sc.sc_trace(trace, wave.port, f"{label}_input")
            port = self.trace_probes[label]
            port.add_trace_internal(trace, f"{label}_discarded")
            assert port.trace_record_count() == 1
            port.discard_trace_records()
            assert port.trace_record_count() == 0
            port.add_trace(trace, f"{label}_member")

    def observe(self, label, event_kind):
        """Record native read/event/edge values and timestamp in a callback."""
        port = self.waves[label].port
        edges = label in ("bool", "logic")
        self.events.append(
            (
                label,
                event_kind,
                str(sc.sc_time_stamp()),
                value_text(port.read()),
                int(bool(port.event())),
                int(bool(port.posedge())) if edges else 0,
                int(bool(port.negedge())) if edges else 0,
            )
        )

    def drive(self):
        """Write a positive/value transition, then a negative/value transition."""
        if self.phase == 0:
            for label, wave in self.waves.items():
                wave.channel.write(make_value(label, 1))
            self.phase = 1
            sc.next_trigger(1, sc.SC_NS)
        elif self.phase == 1:
            for label, wave in self.waves.items():
                wave.channel.write(make_value(label, 2 if label == "wide" else 0))
            self.phase = 2


if sys.argv[1:] == ["--errors"]:
    module_name = sc.sc_module_name("input_port_errors")
    module = sc.sc_module(module_name)
    port = sc.sc_in[bool]("unbound_input")
    del module_name
    try:
        sc.sc_start(sc.SC_ZERO_TIME)
    except Exception as error:
        assert "complete binding failed" in str(error), str(error)
        assert "input_port_errors.unbound_input" in str(error), str(error)
    else:
        raise AssertionError("unbound required sc_in port accepted")
    print("INPUT_PORT_ERRORS|unbound-required")
    raise SystemExit(0)

probe.reset_lifecycle_counts()
module = InputModel("input_ports_top")
assert module.virtual_destruction_verified

with sc.vcd_trace("sc_core_input_ports_public", time_unit=(1, sc.SC_NS)) as trace:
    module.add_traces(trace)
    module.bind_waves()
    for label, wave in module.waves.items():
        assert wave.port.kind() == "sc_in"
        port_value = sc.sc_port_value(wave.port)
        read_value = wave.port.read()
        assert value_text(port_value) == value_text(read_value) == value_text(make_value(label, 0))
        if label == "bool":
            assert type(port_value) is bool
        else:
            assert type(port_value).__cpp_name__ == type(read_value).__cpp_name__
            assert cppyy.addressof(port_value) == cppyy.addressof(read_value)
        assert probe.conversion_value_address(wave.port) == probe.read_value_address(wave.port)
        assert cppyy.addressof(wave.port.default_event()) == cppyy.addressof(wave.port.value_changed_event())
        assert cppyy.addressof(wave.port.value_changed_event()) == cppyy.addressof(wave.channel.value_changed_event())
        if label in ("bool", "logic"):
            assert cppyy.addressof(wave.port.posedge_event()) == cppyy.addressof(wave.channel.posedge_event())
            assert cppyy.addressof(wave.port.negedge_event()) == cppyy.addressof(wave.channel.negedge_event())
            assert not wave.port.posedge() and not wave.port.negedge()
        assert len(module.constructor_ports[label]) == 10
    try:
        sc.sc_port_value(True)
    except TypeError as error:
        assert "sc_port_value requires" in str(error), str(error)
    else:
        raise AssertionError("sc_port_value accepted a Python bool")
    sc.sc_start(2, sc.SC_NS)

for label, trace_probe in module.trace_probes.items():
    assert trace_probe.trace_record_count() == 0
    assert probe.elaboration_count({"wide": 0, "bool": 1, "logic": 2}[label]) == 1
    trace_probe.__destruct__()
    module.trace_probes[label] = None
    assert probe.destruction_count({"wide": 0, "bool": 1, "logic": 2}[label]) == 2

print("INPUT_PORT_CTORS|wide:10/10,bool:10/10,logic:10/10")
print("INPUT_PORT_BIND|wide:7/7,bool:7/7,logic:7/7")
print("INPUT_PORT_ALIASES|wide,bool,logic")
print("INPUT_PORT_FINDERS|bool:cached,logic:cached,wide:cached")
assert all(all(flags) for flags in module.finder_flags.values())
for event in module.events:
    print("INPUT_PORT_EVENT|" + "|".join(map(str, event)))
print("INPUT_PORT_LIFECYCLE|elab=1,1,1;virtual_delete=2,2,2")
print(f"INPUT_PORT_TIME|{sc.sc_time_stamp()}")
