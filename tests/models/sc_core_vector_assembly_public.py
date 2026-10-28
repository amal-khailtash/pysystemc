"""Exercise public sc_vector member assemblies through native cppyy templates."""

import gc
import pathlib

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(pathlib.Path(__file__).resolve().parents[1] / "native" / "sc_core_vector_assembly_probe.h"))
probe = cppyy.gbl.sc_core_vector_assembly_probe
Element = probe.Element
ChannelOwner = probe.ChannelOwner
ElementVector = sc.sc_vector[Element]
SignalVector = sc.sc_vector[sc.sc_signal[bool]]
ChannelVector = sc.sc_vector[ChannelOwner]


def reported_id(action, expected):
    """Return the native report ID for an operation configured to throw.

    Args:
        action: Callable that should raise a SystemC report.
        expected: Native report ID expected from the action.

    Returns:
        The matching native report ID.

    Raises:
        AssertionError: If no report is raised or its ID differs.
    """
    try:
        action()
    except sc.sc_report as report:
        assert report.get_id() == expected
        return report.get_id()
    except TypeError as error:
        # cppyy may wrap a template warning as overload-resolution TypeError.
        assert f"(W{expected})" in str(error)
        return expected
    raise AssertionError(f"operation did not report E{expected}")


assert probe.aliases_match_native_types()
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME

empty_elements = ElementVector("empty_elements")
empty_assembly = probe.input_assembly(empty_elements)
previous_warning_actions = sc.sc_report_handler.set_actions(sc.SC_ID_VECTOR_BIND_EMPTY_, sc.SC_WARNING, sc.SC_THROW)
try:
    empty_assembly_report = reported_id(lambda: empty_assembly.report_empty_bind("sc_in", True), 807)
finally:
    sc.sc_report_handler.set_actions(sc.SC_ID_VECTOR_BIND_EMPTY_, sc.SC_WARNING, previous_warning_actions)

groups = []
cross_copy = None
cross_assigned = None
swapped_left = None
swapped_right = None
for group_index in range(10):
    elements = ElementVector(f"elements_{group_index}", 2)
    inputs = elements.assemble("input") if group_index % 2 == 0 else sc.sc_assemble_vector(elements, "input")
    signals = SignalVector(f"signals_{group_index}", 2)
    spare_signals = SignalVector(f"spare_signals_{group_index}", 2)
    channels = ChannelVector(f"channels_{group_index}", 2)
    for index in range(2):
        value = bool((group_index + index) % 2)
        signals[index].write(value)
        spare_signals[index].write(False)
        channels[index].value.write(value)
    spare = elements.assemble("spare")
    spare.bind(spare_signals)
    channel_members = sc.sc_assemble_vector(channels, "value")

    if group_index == 0:
        # Exercise container binding, copy construction, assignment, swap and
        # mutable/const element access before the simulation starts.
        inputs.bind(signals)
        assembly_type = sc.sc_vector_assembly[f"{Element.__cpp_name__}, sc_core::sc_in<bool>"]
        copied = assembly_type(inputs)
        assert copied.size() == 2 and probe.input_identity(copied, 0, elements)
        assigned = assembly_type(inputs)
        assigned.__assign__(spare)
        assert not probe.input_identity(assigned, 0, elements)
        alternate = sc.sc_assemble_vector(elements, "spare")
        copied.swap(alternate)
        assert not probe.input_identity(copied, 0, elements)
        copied.swap(alternate)

        empty_signals = SignalVector("empty_signals")
        previous_warning_actions = sc.sc_report_handler.set_actions(
            sc.SC_ID_VECTOR_BIND_EMPTY_, sc.SC_WARNING, sc.SC_THROW
        )
        try:
            empty_source_report = reported_id(
                lambda target=spare, source=empty_signals: target.bind(source.begin(), source.end()), 807
            )
            empty_destination_report = reported_id(
                lambda target=spare, source=signals, vector=elements: target.bind(
                    source.begin(), source.end(), vector.end()
                ),
                807,
            )
        finally:
            sc.sc_report_handler.set_actions(sc.SC_ID_VECTOR_BIND_EMPTY_, sc.SC_WARNING, previous_warning_actions)

        const_inputs = probe.const_input_assembly(inputs)
        assert const_inputs.size() == 2 and const_inputs.kind() == "sc_vector_assembly"
        assert const_inputs.name() == elements.name()
        assert const_inputs.at(1).basename() == "input"
        assert const_inputs[0].basename() == "input"
        assert const_inputs.get_elements().size() == 2
        assert inputs.get_elements().size() == 2
        assert probe.input_elements_identity(inputs, elements)
        assert probe.input_elements_identity(const_inputs, elements)
        assert inputs.begin() != inputs.end()
        assert inputs.cbegin() != inputs.cend()
        assert inputs.end() - inputs.begin() == 2
        assert cppyy.addressof(inputs.begin()[0]) == cppyy.addressof(inputs[0])
        assert cppyy.addressof(inputs.begin()[1]) == cppyy.addressof(inputs.at(1))
        assert cppyy.addressof(inputs.cbegin()[1]) == cppyy.addressof(inputs[1])
        assert cppyy.addressof(const_inputs.begin()[0]) == cppyy.addressof(inputs[0])
        assert cppyy.addressof(const_inputs.cbegin()[1]) == cppyy.addressof(inputs.at(1))
        assert probe.spare_identity(assigned, 0, elements)

    elif group_index == 1:
        # Copy, assign, self-assign and swap assemblies across two existing
        # vectors while checking that Python ownership follows native state.
        first_elements, first_inputs = groups[0][:2]
        cross_copy = assembly_type(first_inputs)
        assert cross_copy._vector_owner is first_elements
        assert probe.input_identity(cross_copy, 0, first_elements)
        assert probe.input_elements_identity(cross_copy, first_elements)

        cross_assigned = assembly_type(inputs)
        cross_assigned.__assign__(first_inputs)
        assert cross_assigned._vector_owner is first_elements
        assert probe.input_identity(cross_assigned, 1, first_elements)
        assert probe.input_elements_identity(cross_assigned, first_elements)
        cross_assigned.__assign__(cross_assigned)
        assert cross_assigned._vector_owner is first_elements
        assert probe.input_elements_identity(cross_assigned, first_elements)

        swapped_left = assembly_type(first_inputs)
        swapped_right = assembly_type(inputs)
        swapped_left.swap(swapped_right)
        assert swapped_left._vector_owner is elements
        assert swapped_right._vector_owner is first_elements
        assert probe.input_identity(swapped_left, 0, elements)
        assert probe.input_identity(swapped_right, 0, first_elements)
        assert probe.input_elements_identity(swapped_left, elements)
        assert probe.input_elements_identity(swapped_right, first_elements)

        # Iterator pair overload.
        inputs.bind(signals.begin(), signals.end())
    elif group_index == 2:
        # Iterator pair with member-assembly iterator starting point.
        inputs.bind(signals.begin(), signals.end(), inputs.begin())
    elif group_index == 3:
        # Iterator pair with the underlying sc_vector iterator starting point.
        inputs.bind(signals.begin(), signals.end(), elements.begin())
    elif group_index == 4:
        # Bind an assembly directly to another member assembly.
        inputs.bind(channel_members)
    elif group_index == 5:
        # operator()(container) overload.
        inputs(signals)
    elif group_index == 6:
        # operator()(first, last) overload.
        inputs(signals.begin(), signals.end())
    elif group_index == 7:
        # operator()(first, last, member iterator) overload.
        inputs(signals.begin(), signals.end(), inputs.begin())
    elif group_index == 8:
        # operator()(first, last, base iterator) overload.
        inputs(signals.begin(), signals.end(), elements.begin())
    else:
        # operator()(assembly) overload.
        inputs(channel_members)

    observed = elements.assemble("observed")
    assert observed.size() == 2
    assert probe.observed_identity(observed, 0, elements)
    assert observed.get_elements().size() == 2
    assert probe.channel_elements_identity(channel_members, channels)
    assert probe.channel_address(channel_members, 0) != 0
    groups.append((elements, inputs, observed, signals, spare_signals, channels))

sc.sc_start(1, sc.SC_NS)
assert sc.sc_time_stamp() == sc.sc_time(1, sc.SC_NS)
for group_index, (elements, _inputs, observed, signals, spare_signals, channels) in enumerate(groups):
    for index in range(2):
        expected = bool((group_index + index) % 2)
        assert probe.sample_count(elements, index) == 1
        assert probe.last_sample(elements, index) is expected
        assert observed[index].read() is expected
        assert elements[index].observed.read() is expected

sc.sc_stop()
retained_assemblies = [(row[1], row[2]) for row in groups]
source_owners = [row[3:] for row in groups]
del groups
del elements, inputs, observed, signals, spare_signals, channels, spare, channel_members, _inputs
del first_elements, first_inputs
del copied, assigned, alternate, const_inputs
del cross_copy, cross_assigned, swapped_left, swapped_right
gc.collect()
assert Element.destroyed == 0
del retained_assemblies
gc.collect()
assert Element.destroyed == 20
del source_owners
print(
    "SC_CORE_VECTOR_ASSEMBLY count=10x2 aliases=1 bindings=10 time=1ns "
    f"empty_report={empty_assembly_report},{empty_source_report},{empty_destination_report} destroyed=20"
)
