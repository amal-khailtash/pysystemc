"""Exercise public sc_vector constructors, access, bindings, and ownership."""

import gc
import pathlib

from amal.eda.systemc import sc_core as sc
import cppyy


def _reported_id(action, expected):
    """Run an operation with an expected SystemC report and return its ID.

    Args:
        action: Callable expected to raise ``sc_report``.
        expected: Expected native report ID.

    Returns:
        The report identifier raised by the action.

    Raises:
        AssertionError: If the action does not raise the expected report.
    """
    try:
        action()
    except sc.sc_report as report:
        assert report.get_id() == expected
        return report.get_id()
    except Exception as error:
        # cppyy reports exceptions from templated vector methods as overload
        # diagnostics while preserving the underlying native report ID.
        assert f"(E{expected})" in str(error)
        return expected
    raise AssertionError(f"operation did not report E{expected}")


def _join(values):
    """Return the integer values from a C++ vector as a comma-separated string."""
    return ",".join(str(int(value)) for value in values)


cppyy.include(str(pathlib.Path(__file__).resolve().parents[1] / "native" / "sc_core_vector_container_probe.h"))
probe = cppyy.gbl.sc_core_vector_container_probe
element_vector_type = sc.sc_vector[probe.Element]
element_creator_calls = []


def create_element(name, index):
    """Return a newly allocated module for a Python-backed vector creator.

    Args:
        name: SystemC name generated for this vector element.
        index: Zero-based element index supplied by native sc_vector.

    Returns:
        A native Element whose ownership transfers to its vector.
    """
    element_creator_calls.append((str(name), int(index)))
    return probe.Element(name, index)


default_vector = element_vector_type()
default_vector.init(2)
named_vector = element_vector_type("named_vector")
named_vector.init(2)
sized_vector = element_vector_type("sized_vector", 3)
creator_vector = element_vector_type("creator_vector", 3, create_element)
init_creator_vector = element_vector_type("init_creator_vector")
init_creator_vector.init(3, create_element)

assert default_vector.size() == 2
assert named_vector.size() == 2
assert sized_vector.size() == creator_vector.size() == init_creator_vector.size() == 3
assert default_vector.kind() == "sc_vector"
assert creator_vector.get_elements().size() == 3
assert probe.base_aliases_match()
assert tuple(index for _name, index in element_creator_calls) == (0, 1, 2, 0, 1, 2)

native_creator_vector = element_vector_type("native_creator_vector", 3, probe.ElementCreator())
native_init_creator_vector = element_vector_type("native_init_creator_vector")
native_init_creator_vector.init(3, probe.ElementCreator())
assert tuple(native_creator_vector[index].ordinal for index in range(3)) == (0, 1, 2)
assert tuple(native_init_creator_vector[index].ordinal for index in range(3)) == (0, 1, 2)
destroyed_before_native_creators = int(probe.destroyed_counter())
del native_creator_vector, native_init_creator_vector
gc.collect()
assert int(probe.destroyed_counter()) - destroyed_before_native_creators == 6

creator_vector[0].ordinal = 10
creator_vector.at(1).ordinal = 21
creator_vector.begin()[2].ordinal = 32
assert creator_vector[0].ordinal == 10 and creator_vector.at(1).ordinal == 21
assert creator_vector.end() - creator_vector.begin() == 3
assert creator_vector.cend() - creator_vector.cbegin() == 3
assert creator_vector.begin()[2].ordinal == 32

const_creator_vector = probe.const_vector[probe.Element](creator_vector)
assert const_creator_vector[0].ordinal == 10
assert const_creator_vector.at(1).ordinal == 21
assert creator_vector.get_elements().size() == 3

loose_element = element_vector_type.create_element("loose_element", 0)
assert loose_element.name() == "loose_element"
loose_element.__python_owns__ = False
destroyed_before_loose = int(probe.destroyed_counter())
probe.destroy_element(loose_element)
del loose_element
assert int(probe.destroyed_counter()) == destroyed_before_loose + 1

destroyed_before_vector = int(probe.destroyed_counter())
lifecycle_vector = element_vector_type("lifecycle_vector", 2, create_element)
assert lifecycle_vector.get_elements().size() == 2
del lifecycle_vector
gc.collect()
lifecycle_destroyed = int(probe.destroyed_counter()) - destroyed_before_vector
assert lifecycle_destroyed == 2

old_error_actions = sc.sc_report_handler.set_actions(sc.SC_ID_VECTOR_INIT_CALLED_TWICE_, sc.SC_ERROR, sc.SC_THROW)
try:
    init_report = _reported_id(lambda: sized_vector.init(4), 805)
finally:
    sc.sc_report_handler.set_actions(sc.SC_ID_VECTOR_INIT_CALLED_TWICE_, sc.SC_ERROR, old_error_actions)
assert sized_vector.size() == 3

empty_vector = element_vector_type("empty_vector")
old_warning_actions = sc.sc_report_handler.set_actions(sc.SC_ID_VECTOR_BIND_EMPTY_, sc.SC_WARNING, sc.SC_THROW)
try:
    empty_source_report = _reported_id(lambda: empty_vector.report_empty_bind("sc_in", True), 807)
    empty_destination_report = _reported_id(lambda: creator_vector.report_empty_bind("sc_in", False), 807)
finally:
    sc.sc_report_handler.set_actions(sc.SC_ID_VECTOR_BIND_EMPTY_, sc.SC_WARNING, old_warning_actions)

signal_vector_type = sc.sc_vector[sc.sc_signal[int]]


def create_signal(name, index):
    """Return one initialized native integer signal to its vector owner.

    Args:
        name: SystemC name generated for this signal.
        index: Zero-based element index.

    Returns:
        A native integer signal initialized to seven times its index.
    """
    return sc.sc_signal[int](name, index * 7)


signals = signal_vector_type("python_native_signals", 3, create_signal)
assert signals.size() == 3

fixture = probe.BindFixture("fixture")
fixture.direct_ports.bind(fixture.direct_signals)
fixture.ranged_ports.bind(fixture.ranged_signals.begin(), fixture.ranged_signals.end())
fixture.continued_ports.bind(
    fixture.continued_signals.begin(), fixture.continued_signals.end(), fixture.continued_ports.begin()
)
fixture.call_ports(fixture.call_signals)
fixture.call_ranged_ports(fixture.call_ranged_signals.begin(), fixture.call_ranged_signals.end())
fixture.call_continued_ports(
    fixture.call_continued_signals.begin(), fixture.call_continued_signals.end(), fixture.call_continued_ports.begin()
)
fixture.elements.assemble("direct").bind(fixture.assembled_ports)
fixture.assembled_ports.bind(fixture.assembled_signals)
fixture.elements.assemble("ranged").bind(fixture.assembly_ranged_signals.begin(), fixture.assembly_ranged_signals.end())
fixture.elements.assemble("assembled")(fixture.assembly_call_ports)
fixture.assembly_call_ports.bind(fixture.assembly_call_signals)
fixture.assembly_bind_ports.bind(fixture.channel_elements.assemble("channel"))
fixture.assembly_call_ports_from_channel(fixture.channel_elements.assemble("channel"))

for signal_vector, values in (
    (fixture.direct_signals, (False, True, False)),
    (fixture.ranged_signals, (False, False, True)),
    (fixture.continued_signals, (False, True, False)),
    (fixture.call_signals, (True, True, False)),
    (fixture.call_ranged_signals, (True, False, False)),
    (fixture.call_continued_signals, (False, False, True)),
    (fixture.assembled_signals, (True, False, True)),
    (fixture.assembly_ranged_signals, (False, True, False)),
    (fixture.assembly_call_signals, (True, False, False)),
):
    for index, value in enumerate(values):
        signal_vector[index].write(value)
for index in range(3):
    fixture.channel_elements[index].channel.write(index != 1)

sc.sc_start(sc.SC_ZERO_TIME)
binding_values = tuple(int(value) for value in probe.binding_values(fixture))
assert binding_values == (2, 1, 2, 6, 4, 1, 5, 5, 5, 2, 4), binding_values
signal_values = (int(signals[0].read()), int(signals.at(1).read()), int(signals[2].read()))
assert signal_values == (0, 7, 14)

print(
    "SC_CORE_VECTOR_CONTAINER sizes=2,2,3,3,3 kind="
    f"{default_vector.kind()} elements={creator_vector.get_elements().size()} creator=10,21,32 "
    f"const=10,21 iter=3,3 signal={_join(signal_values)} binds={_join(binding_values)} "
    f"reports={init_report},{empty_source_report},{empty_destination_report} lifecycle=1,2 "
    f"aliases={int(probe.base_aliases_match())}"
)
