"""Compare sc_vector assembly bind and call range contracts with native SystemC."""

import pathlib

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(pathlib.Path(__file__).resolve().parents[1] / "native" / "sc_core_vector_assembly_probe.h"))
probe = cppyy.gbl.sc_core_vector_assembly_probe
ElementVector = sc.sc_vector[probe.Element]
SignalVector = sc.sc_vector[sc.sc_signal[bool]]


def exercise_case(name, destination_size, source_size, destination_start, source_start, source_stop, use_call):
    """Run one range operation and retain its native owners through simulation.

    Args:
        name: Unique SystemC name prefix for this scenario.
        destination_size: Number of destination module ports.
        source_size: Number of source signals.
        destination_start: Destination assembly index where binding begins.
        source_start: Inclusive source signal index.
        source_stop: Exclusive source signal index.
        use_call: Whether to exercise assembly ``operator()`` instead of ``bind``.

    Returns:
        Scenario data used to verify each bound interface and sampled value.
    """
    elements = ElementVector(f"{name}_elements", destination_size)
    sources = SignalVector(f"{name}_sources", source_size)
    fallback = SignalVector(f"{name}_fallback", destination_size)
    for index in range(source_size):
        sources[index].write(bool((index + len(name)) % 2))
    for index in range(destination_size):
        fallback[index].write(False)

    assembly = elements.assemble("input")
    source_first = sources.begin() + source_start
    source_last = sources.begin() + source_stop
    destination_first = assembly.begin() + destination_start
    returned = (
        assembly(source_first, source_last, destination_first)
        if use_call
        else assembly.bind(source_first, source_last, destination_first)
    )
    bound_count = min(source_stop - source_start, destination_size - destination_start)
    return_offset = int(returned - assembly.begin())
    assert return_offset == destination_start + bound_count

    expected_sources = {}
    for offset in range(bound_count):
        expected_sources[destination_start + offset] = source_start + offset
    for destination_index in range(destination_size):
        port = elements[destination_index].input
        elements[destination_index].spare.bind(fallback[destination_index])
        if destination_index in expected_sources:
            source_index = expected_sources[destination_index]
            assert cppyy.addressof(port.get_interface()) == cppyy.addressof(sources[source_index])
        else:
            assert not port.get_interface()
            port.bind(fallback[destination_index])

    return {
        "name": name,
        "elements": elements,
        "sources": sources,
        "fallback": fallback,
        "expected_sources": expected_sources,
        "return_offset": return_offset,
        "bound_count": bound_count,
    }


cases = [
    exercise_case("bind_short_source", 4, 2, 0, 0, 2, False),
    exercise_case("call_short_destination", 2, 4, 0, 0, 4, True),
    exercise_case("bind_subrange", 5, 5, 1, 1, 4, False),
    exercise_case("call_subrange", 4, 5, 2, 1, 5, True),
]

sc.sc_start(1, sc.SC_NS)
assert sc.sc_time_stamp() == sc.sc_time(1, sc.SC_NS)
for case in cases:
    elements = case["elements"]
    for source_index in range(case["sources"].size()):
        expected_source = bool((source_index + len(case["name"])) % 2)
        assert case["sources"][source_index].read() is expected_source
    for destination_index in range(elements.size()):
        source_index = case["expected_sources"].get(destination_index)
        expected_value = bool((source_index + len(case["name"])) % 2) if source_index is not None else False
        assert probe.sample_count(elements, destination_index) == 1
        assert probe.last_sample(elements, destination_index) is expected_value
        assert elements[destination_index].input.read() is expected_value
        if source_index is not None:
            assert case["sources"][source_index].read() is expected_value
        else:
            assert case["fallback"][destination_index].read() is False

sc.sc_stop()
returns = ",".join(str(case["return_offset"]) for case in cases)
bound = ",".join(str(case["bound_count"]) for case in cases)
print(f"SC_CORE_VECTOR_RANGES returns={returns} bound={bound} time=1ns")
