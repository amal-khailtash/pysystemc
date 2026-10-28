"""Compare resolved scalar inout and output ports with native SystemC."""

from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy


PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_core_resolved_scalar_output_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.pysystemc_resolved_scalar_output
logic = dt.sc_logic


def verify_aliases(port_type, base_type):
    """Check all seven public aliases against the expected native base.

    Args:
        port_type: Concrete resolved scalar inout or output port proxy.
        base_type: Native parent port whose inherited aliases are exposed.
    """
    assert port_type.this_type.__cpp_name__ == port_type.__cpp_name__
    assert port_type.data_type.__cpp_name__ == logic.__cpp_name__
    assert port_type.base_type.__cpp_name__ == base_type.__cpp_name__
    for alias in ("in_if_type", "in_port_type", "inout_if_type", "inout_port_type"):
        assert getattr(port_type, alias).__cpp_name__ == getattr(base_type, alias).__cpp_name__


def make_constructor_matrix(label, port_type, channel_factory):
    """Exercise all default, named, interface and parent constructors.

    Args:
        label: Expected native ``kind()`` spelling and object-name prefix.
        port_type: Concrete resolved scalar port class.
        channel_factory: Creates a new resolved scalar channel.

    Returns:
        A pair containing the eight target ports and all retained parents.
    """
    rows = []
    retained = []

    def channel(index):
        result = channel_factory(f"{label}_channel_{index}")
        retained.append(result)
        return result

    port = port_type()
    port.bind(channel(0))
    rows.append(port)

    port = port_type(f"{label}_named")
    port.bind(channel(1))
    rows.append(port)

    rows.append(port_type(channel(2)))
    rows.append(port_type(f"{label}_named_interface", channel(3)))

    for index in (4, 5):
        parent = sc.sc_inout[logic](f"{label}_parent_{index}")
        parent.bind(channel(index))
        retained.append(parent)
        port = port_type(parent) if index == 4 else port_type(f"{label}_named_parent_port", parent)
        rows.append(port)

    for index in (6, 7):
        parent = port_type(f"{label}_concrete_parent_{index}", channel(index))
        retained.append(parent)
        port = port_type(parent) if index == 6 else port_type(f"{label}_named_concrete_parent", parent)
        rows.append(port)

    assert len(rows) == 8
    for port in rows:
        assert port.bind_count() == 1
        assert port.kind() == label
    return rows, retained


def make_assignment_matrix(label, port_type):
    """Call all five native assignment overloads and check self references.

    Args:
        label: Stable object-name prefix for this port family.
        port_type: Concrete resolved scalar inout or output class.

    Returns:
        The five destination ports, their channels and assignment return flags.
    """
    source = sc.sc_signal_resolved(f"{label}_assignment_source", logic("0"))
    input_source = sc.sc_in[logic](f"{label}_assignment_input_source")
    input_source.bind(source)
    inout_source = sc.sc_inout[logic](f"{label}_assignment_inout_source")
    inout_source.bind(source)
    self_source = port_type(f"{label}_assignment_self_source", source)
    retained = [source, input_source, inout_source, self_source]

    ports = []
    channels = []
    returned_self = []
    sources = (logic("1"), source, input_source, inout_source, self_source)
    for index, value in enumerate(sources):
        channel = sc.sc_signal_resolved(f"{label}_assignment_destination_{index}")
        port = port_type(f"{label}_assignment_destination_port_{index}", channel)
        returned = port.__assign__(value)
        channels.append(channel)
        ports.append(port)
        returned_self.append(cppyy.addressof(returned) == cppyy.addressof(port))
    assert all(returned_self), returned_self
    return ports, channels, retained, returned_self


class ResolvedObserver(sc.sc_module):
    """Record value changes from a shared scalar resolved channel."""

    def __init__(self, name, channel):
        """Bind an input port and register a noninitializing observer method."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.input = sc.sc_in_resolved("resolved_input")
        self.input.bind(channel)
        self.transitions = []
        self.sc_method("on_change", self.on_change)
        self.sensitive(self.input)
        self.dont_initialize()

    def on_change(self):
        """Record the resolved scalar value and current simulation time."""
        self.transitions.append((str(sc.sc_time_stamp()), str(self.input.read())))


class ResolvedWriter(sc.sc_module):
    """Write and release one process-specific scalar logic value."""

    def __init__(self, name, port_type, channel, value):
        """Bind an output-family port and register one native writer method."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.output = port_type("resolved_output")
        self.output.bind(channel)
        self.value = logic(value)
        self.initialized = False
        self.sc_method("drive", self.drive)

    def drive(self):
        """Write the initial logic value, then release the writer at one ns."""
        self.output.write(logic("Z") if self.initialized else self.value)
        if not self.initialized:
            self.initialized = True
            sc.next_trigger(1, sc.SC_NS)


if sys.argv[1:] == ["--errors"]:
    """Capture invalid resolved bindings in a fresh independent kernel."""
    error_name = sc.sc_module_name("resolved_scalar_output_errors")
    error_module = sc.sc_module(error_name)
    ordinary_inout = sc.sc_signal[logic]("ordinary_inout_logic")
    ordinary_output = sc.sc_signal[logic]("ordinary_output_logic")
    invalid = (
        (sc.sc_inout_resolved("bad_inout"), ordinary_inout),
        (sc.sc_out_resolved("bad_output"), ordinary_output),
    )
    for port, channel in invalid:
        port.bind(channel)
    reports = []

    def capture(report, actions):
        """Copy a report ID while SystemC still owns the report proxy."""
        reports.append(report.get_id())

    sc.set_report_handler(capture)
    old_actions = sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_DO_NOTHING)
    del error_name
    try:
        sc.sc_start(sc.SC_ZERO_TIME)
    finally:
        sc.sc_report_handler.set_actions(sc.SC_ERROR, old_actions)
        sc.set_report_handler()
    assert reports == [117, 117], reports
    print("RESOLVED_SCALAR_OUTPUT_ERRORS " + "|".join(map(str, reports)))
    del error_module
    sys.exit(0)


probe.reset_counters()
module_name = sc.sc_module_name("resolved_scalar_output_top")
top = sc.sc_module(module_name)
assert probe.verify_virtual_destruction()
destruction_counts = [probe.destruction_count(index) for index in (0, 1)]

inout_type = sc.sc_inout_resolved
output_type = sc.sc_out_resolved
verify_aliases(inout_type, sc.sc_inout[logic])
verify_aliases(output_type, sc.sc_inout_resolved)
inout_matrix, inout_matrix_retained = make_constructor_matrix(
    "sc_inout_resolved", inout_type, lambda name: sc.sc_signal_resolved(name)
)
output_matrix, output_matrix_retained = make_constructor_matrix(
    "sc_out_resolved", output_type, lambda name: sc.sc_signal_resolved(name)
)

inout_assignments, inout_assignment_channels, inout_assignment_retained, inout_assignment_returns = (
    make_assignment_matrix("sc_inout_resolved", inout_type)
)
output_assignments, output_assignment_channels, output_assignment_retained, output_assignment_returns = (
    make_assignment_matrix("sc_out_resolved", output_type)
)

initialized_data = sc.sc_signal_resolved("initialized_data")
initialize_source = sc.sc_signal_resolved("initialize_source", logic("0"))
initialized_interface = sc.sc_signal_resolved("initialized_interface")
init_data_port = inout_type("init_data_port", initialized_data)
init_interface_port = output_type("init_interface_port", initialized_interface)
init_data_port.initialize(logic("1"))
init_interface_port.initialize(initialize_source)

resolved = sc.sc_signal_resolved("resolved_multiwriter", logic("Z"))
observer = ResolvedObserver("resolved_output_observer", resolved)
writer_zero = ResolvedWriter("resolved_output_writer_zero", output_type, resolved, "0")
writer_one = ResolvedWriter("resolved_output_writer_one", inout_type, resolved, "1")

hook_inout = probe.InoutResolvedProbe("hook_inout_resolved")
hook_inout_channel = sc.sc_signal_resolved("hook_inout_channel")
hook_inout.bind(hook_inout_channel)
hook_output = probe.OutResolvedProbe("hook_out_resolved")
hook_output_channel = sc.sc_signal_resolved("hook_output_channel")
hook_output.bind(hook_output_channel)
del module_name

with sc.vcd_trace("resolved_scalar_output_public", time_unit=(1, sc.SC_NS)) as trace:
    sc.sc_trace(trace, resolved, "resolved")
    sc.sc_trace(trace, initialized_data, "initialized_data")
    sc.sc_trace(trace, initialized_interface, "initialized_interface")
    sc.sc_start(2, sc.SC_NS)

assert len(inout_matrix) == len(output_matrix) == 8
assert [port.kind() for port in inout_matrix] == ["sc_inout_resolved"] * 8
assert [port.kind() for port in output_matrix] == ["sc_out_resolved"] * 8
hook_counts = [probe.elaboration_count(index) for index in (0, 1)]
assert hook_counts == [1, 1], hook_counts
assert destruction_counts == [1, 1], destruction_counts
assert len(observer.transitions) == 2, observer.transitions
assert observer.transitions == [("0 s", "X"), ("1 ns", "Z")], observer.transitions
assert str(init_data_port.read()) == "1"
assert str(init_interface_port.read()) == "0"
assignment_values = [str(port.read()) for port in inout_assignments]
output_assignment_values = [str(port.read()) for port in output_assignments]
assert assignment_values == output_assignment_values == ["1", "0", "0", "0", "0"]
assert all(inout_assignment_returns) and all(output_assignment_returns)

print(f"RESOLVED_SCALAR_OUTPUT_CTORS {len(inout_matrix)}|{len(output_matrix)}")
print("RESOLVED_SCALAR_OUTPUT_ALIASES 7|7")
print("RESOLVED_SCALAR_OUTPUT_ASSIGNMENTS 5|5")
print(f"RESOLVED_SCALAR_OUTPUT_INITIALIZE {init_data_port.read()}|{init_interface_port.read()}")
print("RESOLVED_SCALAR_OUTPUT_HOOKS " + "|".join(map(str, hook_counts)))
print("RESOLVED_SCALAR_OUTPUT_DESTRUCTION " + "|".join(map(str, destruction_counts)))
print("RESOLVED_SCALAR_OUTPUT_SAMPLE " + ",".join(f"{time}:{value}" for time, value in observer.transitions))
print("RESOLVED_SCALAR_OUTPUT_ASSIGN_VALUES " + ",".join(assignment_values) + "|" + ",".join(output_assignment_values))
print("RESOLVED_SCALAR_OUTPUTS_OK")
sc.sc_stop()
