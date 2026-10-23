"""Compare concrete buffer and resolved signal behavior with native SystemC."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy


PROBE_HEADER = Path(__file__).resolve().parents[1] / "native" / "sc_core_concrete_signals_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.pysystemc_concrete_signals

# Transfer deletion ownership from the derived proxy to an rv16 base proxy.
probe.reset_rv_destruction_count()
owned_derived = probe.ResolvedVectorDestructionProbe[16]("rv_destruction_probe")
owned_base = cppyy.bind_object(cppyy.addressof(owned_derived), sc.sc_signal_rv[16])
assert type(owned_base).__cpp_name__ == "sc_core::sc_signal_rv<16>"
assert cppyy.addressof(owned_base) == cppyy.addressof(owned_derived)
owned_derived.__python_owns__ = False
owned_base.__python_owns__ = True
del owned_derived
gc.collect()
assert probe.rv_destruction_count() == 0
del owned_base
gc.collect()
rv_destruction_count = probe.rv_destruction_count()
assert rv_destruction_count == 1


class SignalObserver(sc.sc_module):
    """Observe update events from scalar, resolved, and vector channels."""

    def __init__(self, name, buffer, boolean, resolved, vector):
        """Bind input ports and register noninitializing event counters."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.buffer_port = sc.sc_in[bool]("buffer")
        self.boolean_port = sc.sc_in[bool]("boolean")
        self.resolved_port = sc.sc_in[dt.sc_logic]("resolved")
        self.vector_port = sc.sc_in[dt.sc_lv[16]]("vector")
        self.buffer_port.bind(buffer)
        self.boolean_port.bind(boolean)
        self.resolved_port.bind(resolved)
        self.vector_port.bind(vector)
        self.counts = {"buffer": 0, "boolean": 0, "resolved": 0, "vector": 0}
        self.event_flags = {channel: [] for channel in self.counts}
        self.event_deltas = {channel: [] for channel in self.counts}
        self.event_aliases = {
            "buffer": cppyy.addressof(buffer.default_event()) == cppyy.addressof(buffer.value_changed_event()),
            "boolean": cppyy.addressof(boolean.default_event()) == cppyy.addressof(boolean.value_changed_event()),
            "resolved": cppyy.addressof(resolved.default_event()) == cppyy.addressof(resolved.value_changed_event()),
            "vector": cppyy.addressof(vector.default_event()) == cppyy.addressof(vector.value_changed_event()),
        }
        for channel in self.counts:
            self.sc_method(f"on_{channel}", getattr(self, f"on_{channel}"))
            self.sensitive(getattr(self, f"{channel}_port"))
            self.dont_initialize()

    def on_buffer(self):
        """Count a buffer update, including an identical value write."""
        self.counts["buffer"] += 1
        self.event_flags["buffer"].append(self.buffer_port.event())
        self.event_deltas["buffer"].append(sc.sc_delta_count())

    def on_boolean(self):
        """Count a scalar signal value transition."""
        self.counts["boolean"] += 1
        self.event_flags["boolean"].append(self.boolean_port.event())
        self.event_deltas["boolean"].append(sc.sc_delta_count())

    def on_resolved(self):
        """Count a resolved scalar value transition."""
        self.counts["resolved"] += 1
        self.event_flags["resolved"].append(self.resolved_port.event())
        self.event_deltas["resolved"].append(sc.sc_delta_count())

    def on_vector(self):
        """Count a resolved vector value transition."""
        self.counts["vector"] += 1
        self.event_flags["vector"].append(self.vector_port.event())
        self.event_deltas["vector"].append(sc.sc_delta_count())


class MultiWriterModel(sc.sc_module):
    """Write independent native values from two registered SystemC methods."""

    def __init__(self, name, resolved, vector):
        """Retain channels and register one initializer per native writer."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.resolved = resolved
        self.vector = vector
        self.sc_method("writer_zero", self.writer_zero)
        self.sc_method("writer_one", self.writer_one)

    def writer_zero(self):
        """Register the first process-specific resolved driver values."""
        self.resolved.write(dt.sc_logic("0"))
        self.vector.write(dt.sc_lv[16]("0000000000000010"))

    def writer_one(self):
        """Register a conflicting scalar and a Z-valued vector driver."""
        self.resolved.write(dt.sc_logic("1"))
        self.vector.write(dt.sc_lv[16]("00000000000000ZZ"))


def class_aliases(signal_type, expected_value, expected_base):
    """Check concrete class typedefs against expected native C++ types.

    Args:
        signal_type: Instantiated cppyy native signal class.
        expected_value: Native value type alias expected from the family.
        expected_base: Native base type spelling expected from the family.

    Returns:
        A tuple of alias names that can be compared with the native reference.
    """
    this_name = signal_type.this_type.__cpp_name__
    value_name = signal_type.value_type.__cpp_name__
    base_name = signal_type.base_type.__cpp_name__
    assert this_name == signal_type.__cpp_name__
    assert value_name == expected_value
    assert base_name == expected_base
    return this_name, value_name, base_name


def as_text(value):
    """Normalize native byte strings for stable cross-runtime records."""
    if isinstance(value, bool):
        return "1" if value else "0"
    rendered = value.to_string() if hasattr(value, "to_string") else str(value)
    if isinstance(rendered, bytes):
        return rendered.decode()
    return str(rendered)


def assignment_triplet(signal_type, value, interface_source, self_source, prefix):
    """Exercise value, signal-interface, and same-type assignment overloads.

    Args:
        signal_type: Concrete native family constructor.
        value: Native value assigned to the first destination.
        interface_source: Signal channel used for the interface overload.
        self_source: Same concrete family used for the this_type overload.
        prefix: Stable record label.

    Returns:
        Destination channels retained until the kernel update phase.
    """
    from_value = signal_type(f"{prefix}_value")
    from_interface = signal_type(f"{prefix}_interface")
    from_self = signal_type(f"{prefix}_self")
    from_value.__assign__(value)
    from_interface.__assign__(interface_source)
    from_self.__assign__(self_source)
    return from_value, from_interface, from_self


buffer_type = sc.sc_buffer[bool]
rv_type = sc.sc_signal_rv[16]
logic_type = dt.sc_logic
vector_type = dt.sc_lv[16]
wide_vector_type = dt.sc_lv[137]

assert class_aliases(
    buffer_type,
    "sc_core::sc_buffer<bool,sc_core::SC_ONE_WRITER>::value_type",
    "sc_core::sc_signal<bool,sc_core::SC_ONE_WRITER>",
) == (
    buffer_type.__cpp_name__,
    "sc_core::sc_buffer<bool,sc_core::SC_ONE_WRITER>::value_type",
    "sc_core::sc_signal<bool,sc_core::SC_ONE_WRITER>",
)
assert class_aliases(
    sc.sc_signal_resolved,
    "sc_dt::sc_logic",
    "sc_core::sc_signal<sc_dt::sc_logic,sc_core::SC_MANY_WRITERS>",
) == (
    "sc_core::sc_signal_resolved",
    "sc_dt::sc_logic",
    "sc_core::sc_signal<sc_dt::sc_logic,sc_core::SC_MANY_WRITERS>",
)
assert class_aliases(
    rv_type,
    "sc_dt::sc_lv<16>",
    "sc_core::sc_signal<sc_dt::sc_lv<16>,sc_core::SC_MANY_WRITERS>",
) == (
    rv_type.__cpp_name__,
    "sc_dt::sc_lv<16>",
    "sc_core::sc_signal<sc_dt::sc_lv<16>,sc_core::SC_MANY_WRITERS>",
)

# Each family exposes default, name-only, and name-plus-initial-value forms.
buffer_default = buffer_type()
buffer_named = buffer_type("buffer_named")
buffer_initial = buffer_type("buffer_initial", True)
resolved_default = sc.sc_signal_resolved()
resolved_named = sc.sc_signal_resolved("resolved_named")
resolved_initial = sc.sc_signal_resolved("resolved_initial", logic_type("Z"))
rv_default = rv_type()
rv_named = rv_type("rv_named")
rv_initial = rv_type("rv_initial", vector_type("01XZ"))
wide_initial = ("01XZ" * 35)[:137]
wide_next = ("Z10X" * 35)[:137]
rv137 = sc.sc_signal_rv[137]("rv137", wide_vector_type(wide_initial))

constructor_rows = (
    ("buffer", buffer_default, False),
    ("buffer", buffer_named, False),
    ("buffer", buffer_initial, True),
    ("resolved", resolved_default, logic_type("X")),
    ("resolved", resolved_named, logic_type("X")),
    ("resolved", resolved_initial, logic_type("Z")),
    ("rv16", rv_default, vector_type("X" * 16)),
    ("rv16", rv_named, vector_type("X" * 16)),
    ("rv16", rv_initial, vector_type("01XZ")),
)
for family, channel, expected in constructor_rows:
    actual = channel.read()
    if family == "rv16":
        actual = as_text(actual)
        expected = as_text(expected)
    elif family == "resolved":
        actual = as_text(actual)
        expected = as_text(expected)
    assert actual == expected, (family, channel.name(), actual, expected)
    assert channel.kind() == {"buffer": "sc_buffer", "resolved": "sc_signal_resolved", "rv16": "sc_signal_rv"}[family]

# A custom native payload demonstrates that sc_buffer's template stays open.
payload_type = probe.Payload
payload_initial = payload_type(3)
payload_buffer = sc.sc_buffer[payload_type]("payload_buffer", payload_initial)
assert payload_buffer.read().value == 3
payload_buffer.write(payload_type(9))
assert payload_buffer.read().value == 3

# Separate destinations keep each overloaded assignment independently visible.
buffer_if_source = sc.sc_signal[bool]("buffer_if_source", True)
buffer_self_source = buffer_type("buffer_self_source", True)
buffer_assignments = assignment_triplet(buffer_type, True, buffer_if_source, buffer_self_source, "buffer_assign")
resolved_if_source = sc.sc_signal[logic_type]("resolved_if_source", logic_type("1"))
resolved_self_source = sc.sc_signal_resolved("resolved_self_source", logic_type("Z"))
resolved_assignments = assignment_triplet(
    sc.sc_signal_resolved,
    logic_type("0"),
    resolved_if_source,
    resolved_self_source,
    "resolved_assign",
)
rv_if_source = sc.sc_signal[vector_type]("rv_if_source", vector_type("1010"))
rv_self_source = rv_type("rv_self_source", vector_type("ZZ01"))
rv_assignments = assignment_triplet(rv_type, vector_type("01XZ"), rv_if_source, rv_self_source, "rv_assign")

buffer = probe.BufferProbe[bool]("buffer_observed")
boolean = sc.sc_signal[bool]("boolean_observed")
resolved = probe.ResolvedProbe("resolved_observed", logic_type("Z"))
vector = probe.ResolvedVectorProbe[16]("vector_observed", vector_type("0"))
observer = SignalObserver("signal_observer", buffer, boolean, resolved, vector)
resolved_multi = probe.ResolvedProbe("resolved_multi", logic_type("Z"))
vector_multi = probe.ResolvedVectorProbe[16]("vector_multi", vector_type("Z"))
multi_writer = MultiWriterModel("multi_writer", resolved_multi, vector_multi)

with sc.vcd_trace("concrete_signals", time_unit=(1, sc.SC_NS)) as trace:
    for channel, trace_name in (
        (buffer, "buffer"),
        (boolean, "boolean"),
        (resolved, "resolved"),
        (vector, "vector16"),
    ):
        sc.sc_trace(trace, channel, trace_name)
    sc.sc_trace(trace, resolved_multi, "resolved_multi")
    sc.sc_trace(trace, vector_multi, "vector_multi")
    sc.sc_trace(trace, rv137, "vector137")
    # First evaluation checks deferred visibility and dispatches one update per channel.
    buffer.write(True)
    boolean.write(True)
    resolved.write(logic_type("X"))
    vector.write(vector_type("01XZ"))
    rv137.write(wide_vector_type(wide_next))
    payload_buffer.write(payload_type(9))
    assert not buffer.read() and not boolean.read()
    assert as_text(resolved.read()) == "Z"
    assert as_text(vector.read()) == as_text(vector_type("0"))
    assert as_text(rv137.read()) == wide_initial
    sc.sc_start(1, sc.SC_NS)
    assert observer.counts == {"buffer": 1, "boolean": 1, "resolved": 1, "vector": 1}
    assert buffer.update_count == 1 and resolved.update_count == 1 and vector.update_count == 1
    assert buffer.current_storage() and buffer.pending_storage()
    assert as_text(resolved.current_storage()) == "X" and as_text(resolved.pending_storage()) == "X"
    assert as_text(vector.current_storage()) == as_text(vector_type("01XZ"))
    assert as_text(vector.pending_storage()) == as_text(vector_type("01XZ"))
    assert resolved.driver_count() == 1 and vector.driver_count() == 1
    assert payload_buffer.read().value == 9
    assert as_text(resolved_multi.read()) == "X"
    assert as_text(vector_multi.read()) == "0000000000000010"
    assert resolved_multi.driver_count() == 2 and vector_multi.driver_count() == 2
    assert resolved_multi.process_count() == 2 and vector_multi.process_count() == 2
    assert as_text(rv137.read()) == wide_next

    # sc_buffer notifies for an identical write; regular signals suppress it.
    buffer.write(True)
    boolean.write(False)
    resolved.write(logic_type("0"))
    vector.write(vector_type("0000000000000010"))
    rv137.write(wide_vector_type(wide_initial))
    sc.sc_start(1, sc.SC_NS)
    assert observer.counts == {"buffer": 2, "boolean": 2, "resolved": 2, "vector": 2}
    assert buffer.update_count == 2 and resolved.update_count == 2 and vector.update_count == 2
    buffer.write(True)
    boolean.write(False)
    resolved.write(logic_type("0"))
    vector.write(vector_type("0000000000000010"))
    sc.sc_start(1, sc.SC_NS)
    assert observer.counts == {"buffer": 3, "boolean": 2, "resolved": 2, "vector": 2}
    assert buffer.update_count == 3 and resolved.update_count == 2 and vector.update_count == 2
    assert observer.event_aliases == {"buffer": True, "boolean": True, "resolved": True, "vector": True}
    assert observer.event_flags == {
        "buffer": [True, True, True],
        "boolean": [True, True],
        "resolved": [True, True],
        "vector": [True, True],
    }
    assert all(deltas == sorted(deltas) for deltas in observer.event_deltas.values())
    assert len({deltas[0] for deltas in observer.event_deltas.values()}) == 1
    assert len({deltas[1] for deltas in observer.event_deltas.values() if len(deltas) > 1}) == 1
    assert observer.event_deltas["buffer"][2] > observer.event_deltas["buffer"][1]

    # All three assignment overload forms become visible after the update phase.
    sc.sc_start(1, sc.SC_NS)

text = Path("concrete_signals.vcd").read_text()
trace_variables = {
    fields[4]: fields[3] for line in text.splitlines() if line.startswith("$var") for fields in (line.split(),)
}
assert {"buffer", "boolean", "resolved", "vector16", "vector_multi", "vector137"} <= trace_variables.keys()
assert len(trace_variables["vector137"]) > 0
assert (
    next(line.split()[2] for line in text.splitlines() if line.startswith("$var") and line.split()[4] == "vector137")
    == "137"
)
assert "#1" in text


def scalar_trace_values(identifier):
    """Collect VCD scalar values for one declared identifier."""
    return {line[0] for line in text.splitlines() if len(line) > 1 and line[0] in "01xz" and line[1:] == identifier}


def vector_trace_values(identifier):
    """Collect VCD vector values for one declared identifier."""
    return {
        fields[0][1:].lower()
        for line in text.splitlines()
        if line.startswith("b")
        for fields in (line.split(),)
        if len(fields) == 2 and fields[1] == identifier
    }


assert scalar_trace_values(trace_variables["buffer"]) == {"1"}
assert scalar_trace_values(trace_variables["boolean"]) == {"0", "1"}
assert scalar_trace_values(trace_variables["resolved"]) == {"0", "x"}
assert {"1xz", "10"} <= vector_trace_values(trace_variables["vector16"])
assert {wide_initial.lstrip("0").lower(), wide_next.lower()} <= vector_trace_values(trace_variables["vector137"])


def emit_constructor(family, index, channel):
    """Print a stable constructor record after normalizing value formatting."""
    value = channel.read()
    rendered = as_text(value)
    print(f"CONCRETE_CTOR {family} {index} {channel.kind()} {rendered}")


for family, group in (
    ("buffer", (buffer_default, buffer_named, buffer_initial)),
    ("resolved", (resolved_default, resolved_named, resolved_initial)),
    ("rv16", (rv_default, rv_named, rv_initial)),
):
    for index, channel in enumerate(group):
        emit_constructor(family, index, channel)

for family, channels in (
    ("buffer", buffer_assignments),
    ("resolved", resolved_assignments),
    ("rv16", rv_assignments),
):
    values = [channel.read() for channel in channels]
    rendered = [as_text(value) for value in values]
    print(f"CONCRETE_ASSIGN {family} " + "|".join(rendered))

print(
    "CONCRETE_EVENTS "
    f"{observer.counts['buffer']}|{observer.counts['boolean']}|{observer.counts['resolved']}|"
    f"{observer.counts['vector']}|{buffer.update_count}|{resolved.update_count}|{vector.update_count}"
)
print(
    "CONCRETE_DELTA "
    + "|".join(",".join(str(delta) for delta in observer.event_deltas[channel]) for channel in observer.counts)
)
print(f"CONCRETE_PAYLOAD {payload_buffer.read().value}")
print(f"CONCRETE_DESTRUCTION 0|{rv_destruction_count}")
print(f"CONCRETE_WIDE 137 {as_text(rv137.read())}")
print("CONCRETE_SIGNALS_OK")
sc.sc_stop()
