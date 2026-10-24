"""Exercise generic, bool and logic inout/output port behavior in SystemC."""

from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy

PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_core_inout_ports_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.pysystemc_inout_ports
WIDE_WIDTH = 137
WIDE_TYPE = dt.sc_bv[WIDE_WIDTH]


def make_value(label, seed):
    """Create a deterministic native value for one family and source role.

    Args:
        label: ``wide``, ``bool`` or ``logic`` value family.
        seed: Identifies a distinct source value.

    Returns:
        A native SystemC value suitable for a matching signal or port.
    """
    if label == "bool":
        return bool(seed % 2)
    if label == "logic":
        return dt.sc_logic(("0", "1", "X", "Z")[seed % 4])
    bits = ["1" if (index * 11 + seed * 7) % 19 < 8 else "0" for index in range(WIDE_WIDTH)]
    bits[0] = "1" if seed % 2 else "0"
    bits[-1] = "0" if seed % 2 else "1"
    return WIDE_TYPE("".join(bits))


def value_text(value):
    """Render bool, logic and arbitrary-width vector values as native text."""
    if isinstance(value, bool):
        return "1" if value else "0"
    if hasattr(value, "to_string"):
        rendered = value.to_string()
        return rendered.decode() if isinstance(rendered, bytes) else str(rendered)
    return str(value)


def signal_type(value_type):
    """Return the unrestricted native signal template specialization."""
    return sc.sc_signal[value_type]


def make_constructor_matrix(label, value_type, port_type):
    """Construct and bind all eight inout or output constructor forms.

    Args:
        label: Stable name prefix and native kind spelling.
        value_type: bool, sc_logic or sc_bv[137].
        port_type: Matching sc_inout[T] or sc_out[T] template specialization.

    Returns:
        All constructed ports and interface/parent owners needed through simulation.
    """
    value_label = label.removesuffix("_out")
    expected_kind = "sc_out" if label.endswith("_out") else "sc_inout"
    signal = signal_type(value_type)
    retained = []
    rows = []

    def channel(index):
        result = signal(f"{label}_ctor_signal_{index}", make_value(value_label, 0))
        retained.append(result)
        return result

    port = port_type()
    port.bind(channel(0))
    rows.append(port)

    port = port_type(f"{label}_ctor_named")
    port.bind(channel(1))
    rows.append(port)

    rows.append(port_type(channel(2)))
    rows.append(port_type(f"{label}_ctor_named_interface", channel(3)))

    for index in (4, 5):
        parent = sc.sc_inout[value_type](f"{label}_ctor_parent_{index}")
        parent.bind(channel(index))
        retained.append(parent)
        rows.append(port_type(parent) if index == 4 else port_type(f"{label}_ctor_named_parent", parent))

    for index in (6, 7):
        parent = port_type(f"{label}_ctor_this_parent_{index}")
        parent.bind(channel(index))
        retained.append(parent)
        rows.append(port_type(parent) if index == 6 else port_type(f"{label}_ctor_named_this_parent", parent))

    assert len(rows) == 8
    assert all(port.bind_count() == 1 for port in rows)
    assert [port.kind() for port in rows] == [expected_kind] * 8
    return rows, retained


def check_aliases(label, value_type):
    """Check every public inout and output typedef against its native base."""
    inout_type = sc.sc_inout[value_type]
    out_type = sc.sc_out[value_type]
    if label == "bool":
        assert inout_type.data_type.__cpp_name__.endswith("::data_type")
        assert out_type.data_type.__cpp_name__.endswith("::data_type")
    else:
        assert inout_type.data_type.__cpp_name__ == value_type.__cpp_name__
        assert out_type.data_type.__cpp_name__ == value_type.__cpp_name__
    assert inout_type.this_type.__cpp_name__ == inout_type.__cpp_name__
    assert inout_type.inout_if_type.__cpp_name__ == inout_type.if_type.__cpp_name__
    assert inout_type.inout_port_type.__cpp_name__ == inout_type.base_type.__cpp_name__
    assert inout_type.in_if_type.__cpp_name__.startswith("sc_core::sc_signal_in_if<")
    assert inout_type.in_port_type.__cpp_name__.startswith("sc_core::sc_port<")
    assert out_type.this_type.__cpp_name__ == out_type.__cpp_name__
    assert out_type.base_type.__cpp_name__ == inout_type.__cpp_name__
    for alias in ("in_if_type", "in_port_type", "inout_if_type", "inout_port_type"):
        assert getattr(out_type, alias).__cpp_name__ == getattr(inout_type, alias).__cpp_name__


def make_assignment_matrix(label, value_type, port_type):
    """Build five target ports and sources for each assignment overload.

    Args:
        label: bool, logic or wide family name.
        value_type: Native data type used by channels and ports.
        port_type: sc_inout[T] or sc_out[T].

    Returns:
        Targets, target signals, source owners and return-identity results.
    """
    value_label = label.removesuffix("_out")
    signal = signal_type(value_type)
    retained = []
    target_signals = []
    targets = []
    for index in range(5):
        channel = signal(f"{label}_assign_target_{index}")
        port = port_type(f"{label}_assign_target_port_{index}")
        port.bind(channel)
        target_signals.append(channel)
        targets.append(port)

    interface_source = signal(f"{label}_assign_interface", make_value(value_label, 1))
    input_source_channel = signal(f"{label}_assign_input_channel", make_value(value_label, 2))
    inout_source_channel = signal(f"{label}_assign_inout_channel", make_value(value_label, 3))
    self_source_channel = signal(f"{label}_assign_self_channel", make_value(value_label, 4))
    input_source = sc.sc_in[value_type](f"{label}_assign_input_port")
    input_source.bind(input_source_channel)
    inout_source = sc.sc_inout[value_type](f"{label}_assign_inout_port")
    inout_source.bind(inout_source_channel)
    self_source = port_type(f"{label}_assign_this_port")
    self_source.bind(self_source_channel)
    retained.extend((interface_source, input_source_channel, inout_source_channel, self_source_channel))
    retained.extend((input_source, inout_source, self_source))

    def apply():
        """Select each native assignment overload and return self references."""
        sources = (
            make_value(value_label, 0),
            interface_source,
            input_source,
            inout_source,
            self_source,
        )
        returned_self = []
        for target, source in zip(targets, sources, strict=True):
            returned = target.__assign__(source)
            returned_self.append(cppyy.addressof(returned) == cppyy.addressof(target))
        return returned_self

    return targets, target_signals, retained, apply


class WavePair:
    """Hold one inout port, output port and their independently driven channels."""

    def __init__(self, label, value_type):
        """Construct unbound named ports and values for a prebinding trace call."""
        self.label = label
        self.value_type = value_type
        self.signal = signal_type(value_type)
        self.inout_channel = self.signal(f"{label}_wave_inout_channel", make_value(label, 0))
        self.out_channel = self.signal(f"{label}_wave_out_channel", make_value(label, 0))
        self.inout = sc.sc_inout[value_type](f"{label}_wave_inout")
        self.out = sc.sc_out[value_type](f"{label}_wave_out")

    def bind(self):
        """Bind both deferred trace ports to their stable channels."""
        self.inout.bind(self.inout_channel)
        self.out.bind(self.out_channel)

    def write(self, seed):
        """Write one value to each channel through its distinct output port."""
        value = make_value(self.label, 0 if self.label == "logic" and seed == 2 else seed)
        self.inout.write(value)
        self.out.write(value)


class PortModel(sc.sc_module):
    """Exercise public port operations in one native module hierarchy."""

    def __init__(self, name):
        """Create constructor/assignment matrices and timed value-change callbacks."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.types = {"wide": WIDE_TYPE, "bool": bool, "logic": dt.sc_logic}
        self.inout_constructors = {}
        self.out_constructors = {}
        self.constructor_owners = {}
        self.inout_assignments = {}
        self.out_assignments = {}
        self.assignment_owners = {}
        self.assignment_apply = {}
        self.init_data = {}
        self.init_interface = {}
        self.init_sources = {}
        self.init_channels = {}
        self.out_probes = {}
        self.out_probe_channels = {}
        self.waves = {}

        for label, value_type in self.types.items():
            check_aliases(label, value_type)
            inout_rows, inout_owners = make_constructor_matrix(label, value_type, sc.sc_inout[value_type])
            out_rows, out_owners = make_constructor_matrix(label + "_out", value_type, sc.sc_out[value_type])
            self.inout_constructors[label] = inout_rows
            self.out_constructors[label] = out_rows
            self.constructor_owners[label] = (inout_owners, out_owners)

            inout = make_assignment_matrix(label, value_type, sc.sc_inout[value_type])
            output = make_assignment_matrix(label + "_out", value_type, sc.sc_out[value_type])
            self.inout_assignments[label] = (inout[0], inout[1])
            self.out_assignments[label] = (output[0], output[1])
            self.assignment_owners[label] = (inout[2], output[2])
            self.assignment_apply[label] = (inout[3], output[3])

            probe_types = {
                "wide": (probe.InoutWideProbe, probe.OutWideProbe),
                "bool": (probe.InoutBoolProbe, probe.OutBoolProbe),
                "logic": (probe.InoutLogicProbe, probe.OutLogicProbe),
            }
            inout_probe_type, out_probe_type = probe_types[label]
            data_channel = signal_type(value_type)(f"{label}_initialize_data_channel", make_value(label, 0))
            interface_channel = signal_type(value_type)(f"{label}_initialize_interface_channel", make_value(label, 0))
            source_channel = signal_type(value_type)(f"{label}_initialize_source_channel", make_value(label, 3))
            init_data = inout_probe_type(f"{label}_initialize_data")
            init_data.initialize(make_value(label, 2))
            init_data.bind(data_channel)
            init_interface = inout_probe_type(f"{label}_initialize_interface")
            init_interface.initialize(source_channel)
            init_interface.bind(interface_channel)
            out_probe = out_probe_type(f"{label}_out_lifecycle")
            out_probe_channel = signal_type(value_type)(f"{label}_out_lifecycle_channel", make_value(label, 0))
            out_probe.bind(out_probe_channel)
            assert init_data.has_pending_initial_value()
            assert init_interface.has_pending_initial_value()
            self.init_data[label] = init_data
            self.init_interface[label] = init_interface
            self.init_sources[label] = source_channel
            self.init_channels[label] = (data_channel, interface_channel)
            self.out_probes[label] = out_probe
            self.out_probe_channels[label] = out_probe_channel

            self.waves[label] = WavePair(label, value_type)

        self.phase = 0
        self.edge_records = []
        self.return_flags = {}
        self.finder_flags = {}
        self.sc_method("drive", self.drive)
        self._register_edge_methods()
        self.virtual_destruction_verified = probe.verify_virtual_destruction()
        del module_name

    def _register_edge_methods(self):
        """Register value and edge observers on the specialized bool/logic ports."""
        wide_port = self.waves["wide"].inout
        wide_finder = wide_port.value_changed()
        self.finder_flags["wide"] = (cppyy.addressof(wide_finder) == cppyy.addressof(wide_port.value_changed()),)
        self.sc_method("wide_value_changed", lambda: self.observe("wide", "value"))
        self.sensitive(wide_finder)
        self.dont_initialize()

        for label in ("bool", "logic"):
            port = self.waves[label].inout
            value_finder = port.value_changed()
            positive_finder = port.pos()
            negative_finder = port.neg()
            self.finder_flags[label] = (
                cppyy.addressof(value_finder) == cppyy.addressof(port.value_changed()),
                cppyy.addressof(positive_finder) == cppyy.addressof(port.pos()),
                cppyy.addressof(negative_finder) == cppyy.addressof(port.neg()),
            )
            self.sc_method(f"{label}_value_changed", lambda label=label: self.observe(label, "value"))
            self.sensitive(value_finder)
            self.dont_initialize()
            self.sc_method(f"{label}_posedge", lambda label=label: self.observe(label, "pos"))
            self.sensitive(positive_finder)
            self.dont_initialize()
            self.sc_method(f"{label}_negedge", lambda label=label: self.observe(label, "neg"))
            self.sensitive(negative_finder)
            self.dont_initialize()

    def bind_wave_ports(self):
        """Bind VCD-traced ports after prebinding trace registration."""
        for wave in self.waves.values():
            wave.bind()
            for port, channel in (
                (wave.inout, wave.inout_channel),
                (wave.out, wave.out_channel),
            ):
                converted = sc.sc_port_value(port)
                expected = value_text(port.read())
                assert value_text(converted) == expected == value_text(channel.read())
                assert probe.conversion_value_address(port) == probe.read_value_address(port)
                if wave.label != "bool":
                    assert cppyy.addressof(converted) == cppyy.addressof(port.read())

    def add_trace_records(self, trace):
        """Trace every inout/output pair and test deferred cleanup/add_trace hooks."""
        for label, wave in self.waves.items():
            sc.sc_trace(trace, wave.inout, f"{label}_inout")
            sc.sc_trace(trace, wave.out, f"{label}_out")
        for label, wave in self.waves.items():
            self.init_data[label].add_trace_internal(trace, f"{label}_discarded")
            assert self.init_data[label].trace_record_count() == 1
            self.init_data[label].discard_trace_records()
            assert self.init_data[label].trace_record_count() == 0
            self.init_data[label].add_trace(trace, f"{label}_member_add_trace")

    def observe(self, label, event_kind):
        """Store native event flags, data and simulation time from a callback."""
        port = self.waves[label].inout
        self.edge_records.append(
            (
                label,
                event_kind,
                str(sc.sc_time_stamp()),
                value_text(port.read()),
                int(bool(port.event())),
                int(bool(port.posedge())) if label in ("bool", "logic") else 0,
                int(bool(port.negedge())) if label in ("bool", "logic") else 0,
            )
        )

    def drive(self):
        """Apply all port assignments and perform two timed write transitions."""
        if self.phase == 0:
            flags = []
            for label in self.types:
                inout_result = self.assignment_apply[label][0]()
                out_result = self.assignment_apply[label][1]()
                flags.append(all(inout_result) and all(out_result))
            self.return_flags = dict(zip(self.types, flags, strict=True))
            for wave in self.waves.values():
                wave.write(1)
            self.phase = 1
            sc.next_trigger(1, sc.SC_NS)
            return
        if self.phase == 1:
            for wave in self.waves.values():
                wave.write(2)
            self.phase = 2


def run_errors():
    """Verify required unbound-port elaboration fails through native reporting."""
    module_name = sc.sc_module_name("inout_port_errors")
    _module = sc.sc_module(module_name)
    _port = sc.sc_inout[bool]("unbound_inout")
    del module_name
    try:
        sc.sc_start(sc.SC_ZERO_TIME)
    except Exception as error:
        assert "complete binding failed" in str(error), str(error)
        assert "inout_port_errors.unbound_inout" in str(error), str(error)
    else:
        raise AssertionError("unbound required sc_inout port accepted")
    print("INOUT_PORT_ERRORS|unbound-required")


if sys.argv[1:] == ["--errors"]:
    run_errors()
    raise SystemExit(0)

probe.reset_lifecycle_counts()
module = PortModel("inout_ports_top")
assert module.virtual_destruction_verified

with sc.vcd_trace("sc_core_inout_ports_public", time_unit=(1, sc.SC_NS)) as trace:
    module.add_trace_records(trace)
    module.bind_wave_ports()
    for label in module.types:
        inout = module.waves[label].inout
        signal = module.waves[label].inout_channel
        assert inout.kind() == "sc_inout"
        assert cppyy.addressof(inout.default_event()) == cppyy.addressof(inout.value_changed_event())
        assert cppyy.addressof(inout.value_changed_event()) == cppyy.addressof(signal.value_changed_event())
        if label in ("bool", "logic"):
            assert cppyy.addressof(inout.posedge_event()) == cppyy.addressof(signal.posedge_event())
            assert cppyy.addressof(inout.negedge_event()) == cppyy.addressof(signal.negedge_event())
            assert not inout.posedge() and not inout.negedge()
        assert module.inout_constructors[label][0].kind() == "sc_inout"
        assert module.out_constructors[label][0].kind() == "sc_out"
    sc.sc_start(2, sc.SC_NS)

for label in module.types:
    data_value = value_text(module.init_data[label].read())
    interface_value = value_text(module.init_interface[label].read())
    expected_data = value_text(make_value(label, 2))
    expected_interface = value_text(make_value(label, 3))
    assert data_value == expected_data, (label, data_value, expected_data)
    assert interface_value == expected_interface, (label, interface_value, expected_interface)
    assert not module.init_data[label].has_pending_initial_value()
    assert not module.init_interface[label].has_pending_initial_value()
    assert probe.inout_elaboration_count({"wide": 0, "bool": 1, "logic": 2}[label]) == 2
    assert probe.out_elaboration_count({"wide": 0, "bool": 1, "logic": 2}[label]) == 1

for family, label in (
    ("inout", "wide"),
    ("out", "wide"),
    ("inout", "bool"),
    ("out", "bool"),
    ("inout", "logic"),
    ("out", "logic"),
):
    ports, _channels = module.inout_assignments[label] if family == "inout" else module.out_assignments[label]
    actual_values = [value_text(port.read()) for port in ports]
    expected_values = [value_text(make_value(label, seed)) for seed in range(5)]
    assert actual_values == expected_values, (label, family, actual_values, expected_values)
    assert module.return_flags[label]
    print(f"INOUT_PORT_ASSIGN|{family}|{label}|" + ",".join(actual_values) + "|self=11111")

print("INOUT_PORT_CTORS|" + ",".join(f"{label}:8/8" for label in module.types))
print("INOUT_PORT_ALIASES|wide,bool,logic|out-base=inout")
print(
    "INOUT_PORT_INITIALIZE|"
    + ",".join(
        f"{label}:{value_text(module.init_data[label].read())}/{value_text(module.init_interface[label].read())}"
        for label in module.types
    )
)
print("INOUT_PORT_FINDERS|bool:cached,logic:cached,wide:cached")
assert all(all(flags) for flags in module.finder_flags.values())
for event in module.edge_records:
    print("INOUT_PORT_EVENT|" + "|".join(map(str, event)))
print("INOUT_PORT_LIFECYCLE|hooks=2,2,2;out=1,1,1;virtual_delete=1x6")
print(f"INOUT_PORT_TIME|{sc.sc_time_stamp()}")
