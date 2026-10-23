"""Compare resolved scalar and arbitrary-width input ports with SystemC."""

from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy


PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_core_resolved_ports_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.pysystemc_resolved_ports
WIDTH = 137
wide = dt.sc_lv[WIDTH]
logic = dt.sc_logic


def make_input_constructor_matrix(label, port_type, value_type, channel_factory):
    """Exercise every constructor for a resolved input port family.

    Args:
        label: Expected native ``kind()`` result and stable object-name prefix.
        port_type: Concrete resolved scalar or vector input-port type.
        value_type: Scalar or width-137 vector data type for parent ports.
        channel_factory: Creates a channel matching the required resolved type.

    Returns:
        The ten bound ports and all parent/channel objects retained through
        elaboration. Ports borrow both their interface and hierarchy parents.
    """
    rows = []
    retained = []

    def channel(index):
        result = channel_factory(f"{label}_matrix_{index}")
        retained.append(result)
        return result

    def parent_port(index, parent_type):
        result = parent_type(f"{label}_parent_{index}")
        result.bind(channel(index))
        retained.append(result)
        return result

    current = channel(0)
    port = port_type()
    port.bind(current)
    rows.append(port)

    current = channel(1)
    port = port_type(f"{label}_named")
    port.bind(current)
    rows.append(port)

    rows.append(port_type(channel(2)))
    rows.append(port_type(f"{label}_named_interface", channel(3)))

    for index in (4, 5):
        parent = parent_port(index, sc.sc_in[value_type])
        port = port_type(parent) if index == 4 else port_type(f"{label}_named_input_parent", parent)
        rows.append(port)

    for index in (6, 7):
        parent = parent_port(index, sc.sc_inout[value_type])
        port = port_type(parent) if index == 6 else port_type(f"{label}_named_inout_parent", parent)
        rows.append(port)

    for index in (8, 9):
        parent = port_type(f"{label}_parent_{index}", channel(index))
        retained.append(parent)
        port = port_type(parent) if index == 8 else port_type(f"{label}_named_concrete_parent", parent)
        rows.append(port)

    assert len(rows) == 10
    for port in rows:
        assert port.bind_count() == 1
        assert port.kind() == label
    return rows, retained


def verify_aliases(port_type, base_type, data_type):
    """Check all public typedef aliases against their native base types."""
    assert port_type.this_type.__cpp_name__ == port_type.__cpp_name__
    assert port_type.data_type.__cpp_name__ == data_type.__cpp_name__
    for alias in ("in_if_type", "in_port_type", "inout_port_type"):
        assert getattr(port_type, alias).__cpp_name__ == getattr(base_type, alias).__cpp_name__


class ResolvedObserver(sc.sc_module):
    """Observe value changes through the concrete scalar and vector ports."""

    def __init__(self, name, scalar, vector):
        """Bind concrete ports and register noninitializing sensitivity methods."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.scalar = sc.sc_in_resolved("resolved_scalar_input")
        self.vector = sc.sc_in_rv[WIDTH]("resolved_vector_input")
        self.scalar.bind(scalar)
        self.vector.bind(vector)
        self.transitions = {"scalar": [], "vector137": []}
        self.sc_method("on_scalar", self.on_scalar)
        self.sensitive(self.scalar)
        self.dont_initialize()
        self.sc_method("on_vector", self.on_vector)
        self.sensitive(self.vector)
        self.dont_initialize()

    def on_scalar(self):
        """Record scalar resolved changes with their simulation time."""
        self.transitions["scalar"].append((str(sc.sc_time_stamp()), str(self.scalar.read())))

    def on_vector(self):
        """Record width-137 resolved changes with their simulation time."""
        value = self.vector.read().to_string()
        if isinstance(value, bytes):
            value = value.decode()
        else:
            value = str(value)
        self.transitions["vector137"].append((str(sc.sc_time_stamp()), value))


class ResolvedDriver(sc.sc_module):
    """Drive one channel from a process with a delayed release transition."""

    def __init__(self, name, scalar, vector, bit_value):
        """Retain channel proxies and register initial/release methods."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.scalar = scalar
        self.vector = vector
        self.bit_value = bit_value
        self.initialized = False
        self.sc_method("drive_initial", self.drive_initial)

    def drive_initial(self):
        """Drive a process-specific value, then release it one ns later."""
        if self.initialized:
            self.scalar.write(logic("Z"))
            self.vector.write(wide("Z" * WIDTH))
            return
        self.scalar.write(logic(str(self.bit_value)))
        bits = "0" * (WIDTH - 1) + str(self.bit_value)
        self.vector.write(wide(bits))
        self.initialized = True
        sc.next_trigger(1, sc.SC_NS)


if sys.argv[1:] == ["--errors"]:
    """Capture both resolved-port type diagnostics in an independent kernel."""
    error_name = sc.sc_module_name("resolved_input_errors")
    error_module = sc.sc_module(error_name)
    invalid_cases = (
        (sc.sc_in_resolved("bad_scalar_input"), sc.sc_signal[logic]("ordinary_scalar")),
        (sc.sc_in_rv[WIDTH]("bad_vector_input"), sc.sc_signal[wide]("ordinary_vector")),
    )
    for port, channel in invalid_cases:
        port.bind(channel)
    reports = []

    def capture(report, actions):
        """Copy a report ID while the native report proxy is valid."""
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
    print("RESOLVED_PORT_ERRORS " + "|".join(map(str, reports)))
    del error_module
    sys.exit(0)


probe.reset_counters()
module_name = sc.sc_module_name("resolved_ports_top")
top = sc.sc_module(module_name)
assert probe.verify_virtual_destruction()
destruction_counts = [probe.destruction_count(index) for index in range(2)]

in_resolved = sc.sc_in_resolved
in_rv = sc.sc_in_rv[WIDTH]
verify_aliases(in_resolved, sc.sc_in[logic], logic)
verify_aliases(in_rv, sc.sc_in[wide], wide)
assert in_resolved.base_type.__cpp_name__ == "sc_core::sc_in<sc_dt::sc_logic>"
assert in_rv.base_type.__cpp_name__ == f"sc_core::sc_in<sc_dt::sc_lv<{WIDTH}>>"

scalar_matrix, scalar_retained = make_input_constructor_matrix(
    "sc_in_resolved", in_resolved, logic, lambda name: sc.sc_signal_resolved(name)
)
vector_matrix, vector_retained = make_input_constructor_matrix(
    "sc_in_rv", in_rv, wide, lambda name: sc.sc_signal_rv[WIDTH](name)
)

scalar = sc.sc_signal_resolved("resolved_scalar", logic("Z"))
vector = sc.sc_signal_rv[WIDTH]("resolved_vector", wide("Z" * WIDTH))
observer = ResolvedObserver("resolved_observer", scalar, vector)
driver_zero = ResolvedDriver("resolved_driver_zero", scalar, vector, 0)
driver_one = ResolvedDriver("resolved_driver_one", scalar, vector, 1)

hook_scalar = probe.InResolvedProbe("hook_in_resolved")
hook_scalar_channel = sc.sc_signal_resolved("hook_scalar_channel")
hook_scalar.bind(hook_scalar_channel)
hook_vector = probe.InRvProbe[WIDTH]("hook_in_rv")
hook_vector_channel = sc.sc_signal_rv[WIDTH]("hook_vector_channel")
hook_vector.bind(hook_vector_channel)
del module_name

with sc.vcd_trace("resolved_ports_public", time_unit=(1, sc.SC_NS)) as trace:
    sc.sc_trace(trace, scalar, "scalar")
    sc.sc_trace(trace, vector, "vector137")
    sc.sc_start(2, sc.SC_NS)

assert observer.transitions == {
    "scalar": [("0 s", "X"), ("1 ns", "Z")],
    "vector137": [("0 s", "0" * (WIDTH - 1) + "X"), ("1 ns", "Z" * WIDTH)],
}, observer.transitions
hook_counts = [probe.elaboration_count(index) for index in range(2)]
assert hook_counts == [1, 1], hook_counts
assert destruction_counts == [1, 1], destruction_counts

sample_text = "|".join(
    f"{name}:" + ",".join(f"{time}:{value}" for time, value in observer.transitions[name])
    for name in ("scalar", "vector137")
)
print(f"RESOLVED_PORT_CTORS {len(scalar_matrix)}|{len(vector_matrix)}")
print("RESOLVED_PORT_HOOKS " + "|".join(map(str, hook_counts)))
print("RESOLVED_PORT_DESTRUCTION " + "|".join(map(str, destruction_counts)))
print("RESOLVED_PORT_SAMPLE " + sample_text)
print(f"RESOLVED_PORT_WIDE {WIDTH} {vector.read().to_string()}")
print("RESOLVED_PORTS_OK")
sc.sc_stop()
