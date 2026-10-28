"""Exercise arbitrary-width TLM1 ports, finders, analysis ports, and FIFOs."""

import gc
from pathlib import Path
from ctypes import c_int

from amal.eda.systemc import sc_core as sc, tlm

import cppyy


NATIVE = Path(__file__).resolve().parents[1] / "native"
cppyy.include(str(NATIVE / "tlm_ports_analysis_types.h"))
sample_type = cppyy.gbl.tlm_ports_analysis_probe.Sample
# Instantiate each payload specialization before the native fixture creates
# its custom-payload FIFO members.
analysis_fifo_type = tlm.tlm_analysis_fifo[sample_type]
int_analysis_fifo_type = tlm.tlm_analysis_fifo[int]
assert tlm.tlm_analysis_fifo["signed int"] is int_analysis_fifo_type
assert tlm.tlm_analysis_fifo["tlm_ports_analysis_probe::Sample"] is analysis_fifo_type
try:
    tlm.tlm_analysis_fifo["MissingAnalysisPayloadType"]
except TypeError:
    pass
else:
    raise AssertionError("unknown analysis payload type was accepted")
cppyy.include(str(NATIVE / "tlm_ports_analysis_probe.h"))
probe = cppyy.gbl.tlm_ports_analysis_probe
top_type = probe.Top
get_port_type = tlm.tlm_nonblocking_get_port[sample_type]
peek_port_type = tlm.tlm_nonblocking_peek_port[sample_type]
put_port_type = tlm.tlm_nonblocking_put_port[sample_type]
finder_type = tlm.tlm_event_finder_t[probe.Top.get_interface_type, sample_type]
analysis_port_type = tlm.tlm_analysis_port[sample_type]
triple_type = tlm.tlm_analysis_triple[sample_type]


def cpp_name(native_type):
    """Return the C++ declaration name for a cppyy type proxy.

    Args:
        native_type: A cppyy proxy for a concrete native declaration.

    Returns:
        The declaration's fully qualified C++ name.
    """
    return native_type.__cpp_name__


assert cpp_name(get_port_type) == cpp_name(top_type.get_port_type)
assert cpp_name(peek_port_type) == cpp_name(top_type.peek_port_type)
assert cpp_name(put_port_type) == cpp_name(top_type.put_port_type)
assert cpp_name(finder_type) == cpp_name(top_type.event_finder_type)
assert cpp_name(analysis_port_type).startswith("tlm::tlm_analysis_port<")
assert cpp_name(analysis_fifo_type).startswith("tlm::tlm_analysis_fifo<")
assert cpp_name(triple_type).startswith("tlm::tlm_analysis_triple<")
for port_type, alias, interface_type in (
    (get_port_type, "get_if_type", tlm.tlm_nonblocking_get_if[sample_type]),
    (peek_port_type, "peek_if_type", tlm.tlm_nonblocking_peek_if[sample_type]),
    (put_port_type, "put_if_type", tlm.tlm_nonblocking_put_if[sample_type]),
):
    assert hasattr(port_type, alias)
    assert cpp_name(getattr(port_type, alias)) == cpp_name(interface_type)

top = top_type(sc.sc_module_name("top"))
int_fifo = int_analysis_fifo_type()
top.get_port.bind(top.get_store)
top.peek_port.bind(top.peek_store)
top.put_port.bind(top.put_store)

# Construct a finder with the public constructor signature, resolve its bound
# interface event, then release the caller-owned finder before its port dies.
finder = probe.make_get_finder(top.get_port)
finder.__python_owns__ = True
assert cppyy.addressof(finder.find_event()) == cppyy.addressof(top.get_store.ready)
del finder
gc.collect()

# Bind and callable syntax both deliver independent analysis samples in order.
top.analysis_default.bind(top.sink_a)
top.analysis_default(top.sink_b)
top.analysis_named.bind(top.sink_c)
first = sample_type(1, 1001)
second = sample_type(2, 2002)
third = sample_type(3, 3003)
named = sample_type(9, 9009)
top.analysis_default.write(first)
top.analysis_default.write(second)
assert top.analysis_default.unbind(top.sink_b)
assert not top.analysis_default.unbind(top.sink_b)
top.analysis_default.write(third)
top.analysis_named.write(named)

# Cover the default, value, and copy constructors plus both conversion operators.
blank_triple = triple_type()
blank_triple.transaction = sample_type(39, 3939)
assert blank_triple.transaction.sequence == 39
default_fifo_sample = sample_type(40, 4040)
fifo_first = sample_type(50, 5050)
fifo_second = sample_type(60, 6060)
top.fifo_default.write(default_fifo_sample)
top.fifo_named.write(fifo_first)
int_fifo.write(73)
int_fifo.write(tlm.tlm_analysis_triple[int](84))
triple = triple_type(fifo_second)
triple.start_time = sc.sc_time(1, sc.SC_NS)
triple.end_time = sc.sc_time(4, sc.SC_NS)
triple_copy = triple_type(triple)
value_conversion = probe.triple_to_value(triple)
reference_conversion = probe.triple_to_const_reference(triple_copy)
assert value_conversion.sequence == 60
assert reference_conversion.sequence == 60
assert triple_copy.transaction.bits[136].to_bool()
top.fifo_named.write(triple)

sc.sc_start(4, sc.SC_NS)
assert top.get_seen and top.peek_seen and top.put_seen
assert top.get_store.available is False
assert top.peek_store.available is True
assert top.put_store.available is False
assert top.put_store.value.sequence == 30
assert [value.sequence for value in top.sink_a.received] == [1, 2, 3]
assert [value.sequence for value in top.sink_b.received] == [1, 2]
assert [value.sequence for value in top.sink_c.received] == [9]
assert [str(event) for event in top.port_events] == [
    "peek|1|20|202|1",
    "get|1|10|101|1",
    "put|3|30|303|1",
]

default_read = sample_type()
named_read_first = sample_type()
named_read_second = sample_type()
assert top.fifo_default.nb_get(default_read)
assert top.fifo_named.nb_get(named_read_first)
assert top.fifo_named.nb_get(named_read_second)
int_read_first = c_int()
int_read_second = c_int()
assert int_fifo.nb_get(int_read_first)
assert int_fifo.nb_get(int_read_second)
assert default_read.sequence == 40
assert named_read_first.sequence == 50
assert named_read_second.sequence == 60
assert int_read_first.value == 73
assert int_read_second.value == 84
assert triple.start_time == sc.sc_time(1, sc.SC_NS)
assert triple.end_time == sc.sc_time(4, sc.SC_NS)
assert triple_copy.transaction.sequence == 60

for event in top.port_events:
    print("PORT_ANALYSIS_EVENT " + str(event))
print(
    "ANALYSIS_PORT",
    len(top.sink_a.received),
    top.sink_a.received[0].sequence,
    top.sink_a.received[1].sequence,
    len(top.sink_b.received),
    top.sink_b.received[0].sequence,
    len(top.sink_c.received),
    top.sink_c.received[0].sequence,
)
print(
    "ANALYSIS_FIFO",
    default_read.sequence,
    default_read.bits.to_uint64(),
    named_read_first.sequence,
    named_read_second.sequence,
    named_read_second.bits.to_uint64(),
)
print("ANALYSIS_FIFO_INT", int_read_first.value, int_read_second.value)
print(
    "ANALYSIS_TRIPLE",
    triple.start_time,
    triple.transaction.sequence,
    triple.end_time,
    triple_copy.transaction.sequence,
    value_conversion.sequence,
    reference_conversion.sequence,
)
assert len(top.port_events) == 3
# The final loop value is a borrowed native string proxy into the module.
del event
del top
gc.collect()
assert int(top_type.destroyed) == 1
print("PORT_ANALYSIS_LIFETIME 1")
print("TLM_PORTS_ANALYSIS_PUBLIC_OK")
