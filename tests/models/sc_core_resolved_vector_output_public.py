"""Compare resolved vector inout/output ports with an independent native run."""

from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy


PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_core_resolved_vector_output_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.pysystemc_resolved_vector_output
WIDE_WIDTH = 137
SMALL_WIDTH = 5


def value_bits(width, seed):
    """Build the stable data values used to distinguish assignment overloads.

    Args:
        width: Resolved vector width.
        seed: Selects the bit pattern for one overload.

    Returns:
        A string containing four-state logic symbols.
    """
    symbols = "01XZ"
    result = [symbols[(bit + seed) % len(symbols)] for bit in range(width)]
    result[0] = "1" if seed % 2 else "0"
    result[-1] = "0" if seed % 2 else "1"
    return "".join(result)


def as_text(value):
    """Normalize cppyy's native string wrappers to Python text.

    Args:
        value: Native string or a SystemC vector value.

    Returns:
        The stable text representation.
    """
    rendered = value.to_string() if hasattr(value, "to_string") else value
    return rendered.decode() if isinstance(rendered, bytes) else str(rendered)


def make_constructor_matrix(label, width, port_type, value_type, signal_type):
    """Exercise all eight interface and parent constructors for one port type.

    Args:
        label: Stable prefix that also identifies the native ``kind()`` value.
        width: Width of the matching resolved signal.
        port_type: ``sc_inout_rv[width]`` or ``sc_out_rv[width]``.
        value_type: Matching ``sc_lv[width]`` specialization.
        signal_type: Matching ``sc_signal_rv[width]`` specialization.

    Returns:
        The eight ports and all channels and parent ports retained to simulation.
    """
    rows = []
    retained = []

    def channel(index):
        result = signal_type(f"{label}_matrix_{index}")
        retained.append(result)
        return result

    rows.append(port_type())
    rows[-1].bind(channel(0))
    rows.append(port_type(f"{label}_named"))
    rows[-1].bind(channel(1))
    rows.append(port_type(channel(2)))
    rows.append(port_type(f"{label}_named_interface", channel(3)))

    for index in (4, 5):
        parent = sc.sc_inout[value_type](f"{label}_parent_{index}")
        parent.bind(channel(index))
        retained.append(parent)
        rows.append(port_type(parent) if index == 4 else port_type(f"{label}_named_parent", parent))

    for index in (6, 7):
        parent = port_type(f"{label}_this_parent_{index}")
        parent.bind(channel(index))
        retained.append(parent)
        rows.append(port_type(parent) if index == 6 else port_type(f"{label}_named_this_parent", parent))

    assert len(rows) == 8
    for port in rows:
        assert port.bind_count() == 1
        assert port.kind() == label.split("_rv")[0] + "_rv"
    return rows, retained


def verify_aliases(port_type, base_type, value_type):
    """Compare all seven public typedefs with their native base definitions.

    Args:
        port_type: Concrete inout or output port specialization.
        base_type: Its concrete C++ base port specialization.
        value_type: Matching resolved logic-vector specialization.
    """
    assert port_type.this_type.__cpp_name__ == port_type.__cpp_name__
    assert port_type.base_type.__cpp_name__ == base_type.__cpp_name__
    assert port_type.data_type.__cpp_name__ == value_type.__cpp_name__
    for alias in ("in_if_type", "in_port_type", "inout_if_type", "inout_port_type"):
        assert getattr(port_type, alias).__cpp_name__ == getattr(base_type, alias).__cpp_name__


class AssignmentDriver(sc.sc_module):
    """Exercise the five public assignment overloads from a SystemC process."""

    def __init__(self, name, width, port_type, value_type, signal_type, targets, prefix):
        """Retain assignment sources and register their process callback.

        Args:
            name: SystemC module name.
            width: Width of each resolved value.
            port_type: Concrete inout or output port specialization.
            value_type: Matching four-state vector specialization.
            signal_type: Matching resolved signal specialization.
            targets: Five destination ports, one for each operator overload.
            prefix: Unique source and output name prefix.
        """
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.width = width
        self.targets = targets
        self.values = [value_type(value_bits(width, seed)) for seed in range(5)]
        self.interface_source = signal_type(f"{prefix}_interface_source", self.values[1])
        self.input_source = signal_type(f"{prefix}_input_source", self.values[2])
        self.inout_source = signal_type(f"{prefix}_inout_source", self.values[3])
        self.this_source = signal_type(f"{prefix}_this_source", self.values[4])
        self.input_parent = sc.sc_in[value_type](f"{prefix}_input_parent")
        self.input_parent.bind(self.input_source)
        self.inout_parent = sc.sc_inout[value_type](f"{prefix}_inout_parent")
        self.inout_parent.bind(self.inout_source)
        self.this_parent = port_type(f"{prefix}_this_parent")
        self.this_parent.bind(self.this_source)
        self.returned_self = [False] * 5
        self.sc_method("apply", self.apply)

    def apply(self):
        """Assign data, native interface, input, inout and same-type sources."""
        sources = (self.values[0], self.interface_source, self.input_parent, self.inout_parent, self.this_parent)
        for index, (target, source) in enumerate(zip(self.targets, sources, strict=True)):
            result = target.__assign__(source)
            self.returned_self[index] = cppyy.addressof(result) == cppyy.addressof(target)


class VectorObserver(sc.sc_module):
    """Record resolved changes through an input port with native times."""

    def __init__(self, name, width, channel):
        """Bind an input and register a noninitializing value-change method.

        Args:
            name: SystemC module name.
            width: Width of the vector channel.
            channel: Resolved signal retained by the caller.
        """
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.input = sc.sc_in_rv[width](f"{name}_input")
        self.input.bind(channel)
        self.transitions = []
        self.sc_method("sample", self.sample)
        self.sensitive(self.input)
        self.dont_initialize()

    def sample(self):
        """Append the full value and simulation timestamp after an update."""
        self.transitions.append((str(sc.sc_time_stamp()), as_text(self.input.read())))


class VectorWriter(sc.sc_module):
    """Drive a resolved output port and release it after one nanosecond."""

    def __init__(self, name, width, channel, initial):
        """Bind a writer port and register its initial/release method.

        Args:
            name: SystemC module name.
            width: Width of the resolved signal.
            channel: Signal shared with the other writer.
            initial: Per-process vector to drive at time zero.
        """
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.width = width
        self.output = sc.sc_out_rv[width]("output")
        self.output.bind(channel)
        self.initial = sc_dt.sc_lv[width](initial)
        self.initialized = False
        self.sc_method("drive", self.drive)

    def drive(self):
        """Drive the process value once, then release every bit."""
        if self.initialized:
            self.output.write(sc_dt.sc_lv[self.width]("Z" * self.width))
            return
        self.output.write(self.initial)
        self.initialized = True
        sc.next_trigger(1, sc.SC_NS)


def run_errors():
    """Capture report 117 from ordinary width-137 signal bindings."""
    module_name = sc.sc_module_name("resolved_vector_output_errors")
    module = sc.sc_module(module_name)
    value_type = sc_dt.sc_lv[WIDE_WIDTH]
    ordinary_inout = sc.sc_signal[value_type]("inout_ordinary")
    ordinary_out = sc.sc_signal[value_type]("out_ordinary")
    inout_port = sc.sc_inout_rv[WIDE_WIDTH]("bad_inout_rv")
    out_port = sc.sc_out_rv[WIDE_WIDTH]("bad_out_rv")
    inout_port.bind(ordinary_inout)
    out_port.bind(ordinary_out)
    reports = []

    def capture(report, actions):
        """Copy a report ID while its native report proxy remains valid."""
        reports.append(report.get_id())

    sc.set_report_handler(capture)
    previous_actions = sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_DO_NOTHING)
    del module_name
    try:
        sc.sc_start(sc.SC_ZERO_TIME)
    finally:
        sc.sc_report_handler.set_actions(sc.SC_ERROR, previous_actions)
        sc.set_report_handler()
    assert reports == [117, 117], reports
    print("RESOLVED_VECTOR_OUTPUT_ERRORS " + "|".join(map(str, reports)))
    del module


if sys.argv[1:] == ["--errors"]:
    run_errors()
    sys.exit(0)

probe.reset_counters()
module_name = sc.sc_module_name("resolved_vector_output_top")
top = sc.sc_module(module_name)
assert probe.verify_virtual_destruction()
destruction_counts = [probe.destruction_count(index) for index in range(4)]

wide_value = sc_dt.sc_lv[WIDE_WIDTH]
small_value = sc_dt.sc_lv[SMALL_WIDTH]
inout_wide_type = sc.sc_inout_rv[WIDE_WIDTH]
out_wide_type = sc.sc_out_rv[WIDE_WIDTH]
inout_small_type = sc.sc_inout_rv[SMALL_WIDTH]
out_small_type = sc.sc_out_rv[SMALL_WIDTH]
verify_aliases(inout_wide_type, sc.sc_inout[wide_value], wide_value)
verify_aliases(out_wide_type, inout_wide_type, wide_value)
verify_aliases(inout_small_type, sc.sc_inout[small_value], small_value)
verify_aliases(out_small_type, inout_small_type, small_value)

inout_wide_matrix, inout_wide_retained = make_constructor_matrix(
    "sc_inout_rv", WIDE_WIDTH, inout_wide_type, wide_value, sc.sc_signal_rv[WIDE_WIDTH]
)
out_wide_matrix, out_wide_retained = make_constructor_matrix(
    "sc_out_rv", WIDE_WIDTH, out_wide_type, wide_value, sc.sc_signal_rv[WIDE_WIDTH]
)
inout_small_matrix, inout_small_retained = make_constructor_matrix(
    "sc_inout_rv", SMALL_WIDTH, inout_small_type, small_value, sc.sc_signal_rv[SMALL_WIDTH]
)
out_small_matrix, out_small_retained = make_constructor_matrix(
    "sc_out_rv", SMALL_WIDTH, out_small_type, small_value, sc.sc_signal_rv[SMALL_WIDTH]
)


def make_assignment_targets(prefix, width, port_type):
    """Create five distinct bound ports so each assignment has visible output.

    Args:
        prefix: Unique channel and port prefix.
        width: Width for the resolved channels.
        port_type: Concrete inout or output port class.

    Returns:
        The five targets, channels, and port objects retained through simulation.
    """
    channels = []
    ports = []
    for index in range(5):
        channel = sc.sc_signal_rv[width](f"{prefix}_assignment_{index}")
        port = port_type(f"{prefix}_target_{index}")
        port.bind(channel)
        channels.append(channel)
        ports.append(port)
    return ports, channels


inout_wide_targets, inout_wide_assignment_channels = make_assignment_targets("inout_wide", WIDE_WIDTH, inout_wide_type)
out_wide_targets, out_wide_assignment_channels = make_assignment_targets("out_wide", WIDE_WIDTH, out_wide_type)
inout_small_targets, inout_small_assignment_channels = make_assignment_targets(
    "inout_small", SMALL_WIDTH, inout_small_type
)
out_small_targets, out_small_assignment_channels = make_assignment_targets("out_small", SMALL_WIDTH, out_small_type)
inout_wide_assign = AssignmentDriver(
    "inout_wide_assign",
    WIDE_WIDTH,
    inout_wide_type,
    wide_value,
    sc.sc_signal_rv[WIDE_WIDTH],
    inout_wide_targets,
    "inout_wide",
)
out_wide_assign = AssignmentDriver(
    "out_wide_assign", WIDE_WIDTH, out_wide_type, wide_value, sc.sc_signal_rv[WIDE_WIDTH], out_wide_targets, "out_wide"
)
inout_small_assign = AssignmentDriver(
    "inout_small_assign",
    SMALL_WIDTH,
    inout_small_type,
    small_value,
    sc.sc_signal_rv[SMALL_WIDTH],
    inout_small_targets,
    "inout_small",
)
out_small_assign = AssignmentDriver(
    "out_small_assign",
    SMALL_WIDTH,
    out_small_type,
    small_value,
    sc.sc_signal_rv[SMALL_WIDTH],
    out_small_targets,
    "out_small",
)

resolved_wide = sc.sc_signal_rv[WIDE_WIDTH]("resolved_rv137", wide_value("Z" * WIDE_WIDTH))
resolved_small = sc.sc_signal_rv[SMALL_WIDTH]("resolved_rv5", small_value("Z" * SMALL_WIDTH))
wide_pattern = "0" + "Z" * (WIDE_WIDTH - 2) + "0"
small_pattern = "0" + "Z" * (SMALL_WIDTH - 2) + "0"
wide_zero = VectorWriter("wide_zero_writer", WIDE_WIDTH, resolved_wide, wide_pattern)
wide_one = VectorWriter("wide_one_writer", WIDE_WIDTH, resolved_wide, wide_pattern.replace("0", "1"))
small_zero = VectorWriter("small_zero_writer", SMALL_WIDTH, resolved_small, small_pattern)
small_one = VectorWriter("small_one_writer", SMALL_WIDTH, resolved_small, small_pattern.replace("0", "1"))
wide_observer = VectorObserver("wide_observer", WIDE_WIDTH, resolved_wide)
small_observer = VectorObserver("small_observer", SMALL_WIDTH, resolved_small)

initialized_inout_wide = sc.sc_signal_rv[WIDE_WIDTH]("initialized_inout_rv137")
initialized_out_wide = sc.sc_signal_rv[WIDE_WIDTH]("initialized_out_rv137")
initialized_inout_small = sc.sc_signal_rv[SMALL_WIDTH]("initialized_inout_rv5")
initialized_out_small = sc.sc_signal_rv[SMALL_WIDTH]("initialized_out_rv5")
initialize_source_wide = sc.sc_signal_rv[WIDE_WIDTH]("initialize_source_rv137", wide_value(value_bits(WIDE_WIDTH, 1)))
initialize_source_small = sc.sc_signal_rv[SMALL_WIDTH]("initialize_source_rv5", small_value(value_bits(SMALL_WIDTH, 1)))
initialize_inout_wide = inout_wide_type("initialize_inout_rv137", initialized_inout_wide)
initialize_out_wide = out_wide_type("initialize_out_rv137", initialized_out_wide)
initialize_inout_small = inout_small_type("initialize_inout_rv5", initialized_inout_small)
initialize_out_small = out_small_type("initialize_out_rv5", initialized_out_small)
initialize_inout_wide.initialize(wide_value(value_bits(WIDE_WIDTH, 0)))
initialize_out_wide.initialize(initialize_source_wide)
initialize_inout_small.initialize(small_value(value_bits(SMALL_WIDTH, 0)))
initialize_out_small.initialize(initialize_source_small)

hook_inout_wide = probe.InoutRvProbe[WIDE_WIDTH]("hook_inout_rv137")
hook_out_wide = probe.OutRvProbe[WIDE_WIDTH]("hook_out_rv137")
hook_inout_small = probe.InoutRvProbe[SMALL_WIDTH]("hook_inout_rv5")
hook_out_small = probe.OutRvProbe[SMALL_WIDTH]("hook_out_rv5")
hook_inout_wide_channel = sc.sc_signal_rv[WIDE_WIDTH]("hook_inout_wide_channel")
hook_out_wide_channel = sc.sc_signal_rv[WIDE_WIDTH]("hook_out_wide_channel")
hook_inout_small_channel = sc.sc_signal_rv[SMALL_WIDTH]("hook_inout_small_channel")
hook_out_small_channel = sc.sc_signal_rv[SMALL_WIDTH]("hook_out_small_channel")
hook_inout_wide.bind(hook_inout_wide_channel)
hook_out_wide.bind(hook_out_wide_channel)
hook_inout_small.bind(hook_inout_small_channel)
hook_out_small.bind(hook_out_small_channel)

with sc.vcd_trace("resolved_vector_output_public", time_unit=(1, sc.SC_NS)) as trace:
    for prefix, width, channels, resolved, initialized in (
        ("inout137", WIDE_WIDTH, inout_wide_assignment_channels, resolved_wide, initialized_inout_wide),
        ("out137", WIDE_WIDTH, out_wide_assignment_channels, resolved_wide, initialized_out_wide),
        ("inout5", SMALL_WIDTH, inout_small_assignment_channels, resolved_small, initialized_inout_small),
        ("out5", SMALL_WIDTH, out_small_assignment_channels, resolved_small, initialized_out_small),
    ):
        for index, channel in enumerate(channels):
            sc.sc_trace(trace, channel, f"{prefix}_assign_{index}")
        sc.sc_trace(trace, resolved, f"{prefix}_resolved")
        sc.sc_trace(trace, initialized, f"{prefix}_initialized")
    del module_name
    sc.sc_start(2, sc.SC_NS)

hook_counts = [probe.elaboration_count(index) for index in range(4)]
assert hook_counts == [1, 1, 1, 1], hook_counts
assert destruction_counts == [1, 1, 1, 1], destruction_counts
for fixture in (inout_wide_assign, out_wide_assign, inout_small_assign, out_small_assign):
    assert fixture.returned_self == [True] * 5, fixture.returned_self
for fixture, channels in (
    (inout_wide_assign, inout_wide_assignment_channels),
    (out_wide_assign, out_wide_assignment_channels),
    (inout_small_assign, inout_small_assignment_channels),
    (out_small_assign, out_small_assignment_channels),
):
    for index, channel in enumerate(channels):
        expected = value_bits(fixture.width, index)
        assert as_text(channel.read()) == expected
        assert as_text(fixture.values[index]) == expected

expected_wide = "X" + "Z" * (WIDE_WIDTH - 2) + "X"
expected_small = "X" + "Z" * (SMALL_WIDTH - 2) + "X"
assert wide_observer.transitions == [("0 s", expected_wide), ("1 ns", "Z" * WIDE_WIDTH)]
assert small_observer.transitions == [("0 s", expected_small), ("1 ns", "Z" * SMALL_WIDTH)]
assert as_text(resolved_wide.read()) == "Z" * WIDE_WIDTH
assert as_text(resolved_small.read()) == "Z" * SMALL_WIDTH

print("RESOLVED_VECTOR_OUTPUT_CTORS 8|8|8|8")
print("RESOLVED_VECTOR_OUTPUT_HOOKS " + "|".join(map(str, hook_counts)))
print("RESOLVED_VECTOR_OUTPUT_DESTRUCTION " + "|".join(map(str, destruction_counts)))
print("RESOLVED_VECTOR_OUTPUT_ASSIGNMENTS 20")
print(
    "RESOLVED_VECTOR_OUTPUT_SAMPLES wide:"
    + ",".join(value for _time, value in wide_observer.transitions)
    + "|small:"
    + ",".join(value for _time, value in small_observer.transitions)
)
print(
    "RESOLVED_VECTOR_OUTPUT_INITIALIZE "
    + "|".join(
        as_text(signal.read())
        for signal in (initialized_inout_wide, initialized_out_wide, initialized_inout_small, initialized_out_small)
    )
)
print("RESOLVED_VECTOR_OUTPUT_OK")
sc.sc_stop()
