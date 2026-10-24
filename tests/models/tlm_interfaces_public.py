"""Exercise TLM interface inheritance, wide values and native adapter calls."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy
from pathlib import Path


PROBE_HEADER = Path(__file__).resolve().parents[1] / "native/tlm_interfaces_probe.h"
cppyy.include(str(PROBE_HEADER))
probe = cppyy.gbl.pysystemc_tlm_interfaces


def make_wide_sample(value):
    """Create a 137-bit unsigned value with its high bit and low payload set."""
    result = sc_dt.sc_biguint[137](value)
    result[136] = True
    return result


def wide_record(value):
    """Return low-word and high-bit observations for one 137-bit value."""
    return int(value.to_uint64()), int(bool(value[136]))


sample = make_wide_sample(17)
wide = probe.make_wide_value_adapter(sample)
wide_type = sc_dt.sc_biguint[137]
protocol_type = tlm.tlm_get_peek_if[sc_dt.sc_biguint[137]]
assert isinstance(wide, protocol_type)
assert isinstance(wide, tlm.tlm_blocking_get_if[sc_dt.sc_biguint[137]])
assert isinstance(wide, tlm.tlm_nonblocking_get_if[sc_dt.sc_biguint[137]])
assert isinstance(wide, tlm.tlm_blocking_peek_if[sc_dt.sc_biguint[137]])
assert isinstance(wide, tlm.tlm_nonblocking_peek_if[sc_dt.sc_biguint[137]])
for interface in (
    tlm.tlm_get_if,
    tlm.tlm_peek_if,
    tlm.tlm_blocking_get_peek_if,
    tlm.tlm_nonblocking_get_peek_if,
):
    assert isinstance(wide, interface[wide_type])
tag = tlm.tlm_tag[wide_type]()
assert isinstance(tag, tlm.tlm_tag[wide_type])
can_get_before = wide.nb_can_get(tag)
can_peek_before = wide.nb_can_peek(tag)
get_event_address = cppyy.addressof(wide.ok_to_get(tag))
peek_event_address = cppyy.addressof(wide.ok_to_peek(tag))
same_get_event = get_event_address == cppyy.addressof(wide.ok_to_get())
same_peek_event = peek_event_address == cppyy.addressof(wide.ok_to_peek())
assert can_get_before and can_peek_before and same_get_event and same_peek_event

nonblocking_peek_output = wide_type(0)
assert wide.nb_peek(nonblocking_peek_output)
wide_peek = wide.peek(tag)
blocking_peek_output = wide_type(0)
wide.peek(blocking_peek_output)
can_get_after_output_peek = wide.nb_can_get(tag)
assert can_get_after_output_peek, "blocking peek output overload must not consume the sample"
wide_get = wide.get(tag)

probe.reset_wide_value_adapter(wide, sample)
blocking_get_output = wide_type(0)
wide.get(blocking_get_output)
probe.reset_wide_value_adapter(wide, sample)
nonblocking_get_output = wide_type(0)
assert wide.nb_get(nonblocking_get_output)
failed_get_output = make_wide_sample(91)
get_sentinel = wide_type(failed_get_output)
failed_peek_output = make_wide_sample(93)
peek_sentinel = wide_type(failed_peek_output)
assert not wide.nb_get(failed_get_output) and wide_record(failed_get_output) == wide_record(get_sentinel)
assert not wide.nb_peek(failed_peek_output) and wide_record(failed_peek_output) == wide_record(peek_sentinel)
can_get_after = wide.nb_can_get(tag)
can_peek_after = wide.nb_can_peek(tag)
assert not can_get_after and not can_peek_after
wide_values = (
    *wide_record(nonblocking_peek_output),
    *wide_record(wide_peek),
    *wide_record(blocking_peek_output),
    *wide_record(wide_get),
    *wide_record(blocking_get_output),
    *wide_record(nonblocking_get_output),
    *wide_record(failed_get_output),
    *wide_record(failed_peek_output),
    int(can_get_before),
    int(can_peek_before),
    int(can_get_after_output_peek),
    int(can_get_after),
    int(can_peek_after),
    int(same_get_event),
    int(same_peek_event),
    probe.wide_get_calls(wide),
    probe.wide_peek_calls(wide),
)
print("TLM_IFACE_WIDE", *wide_values)


record_type = probe.Record
record = record_type()
record.code = 37
later = record_type()
later.code = 42
writer = probe.CustomWriteAdapter()
analysis_interface = tlm.tlm_analysis_if[record_type]
delayed_interface = tlm.tlm_delayed_analysis_if[record_type]
assert isinstance(writer, analysis_interface)
assert isinstance(writer, tlm.tlm_write_if[record_type])
assert isinstance(writer, delayed_interface)
assert isinstance(writer, tlm.tlm_delayed_write_if[record_type])
writer.write(record)
writer.write(later, sc.sc_time(3, sc.SC_NS))
record.code = -1
later.code = -2
assert writer.immediate().code == 37 and writer.delayed().code == 42
assert writer.immediate_calls() == writer.delayed_calls() == 1
assert writer.delay() == sc.sc_time(3, sc.SC_NS)
print(
    "TLM_IFACE_WRITE",
    writer.immediate().code,
    writer.delayed().code,
    writer.immediate_calls(),
    writer.delayed_calls(),
    int(writer.delay().value()),
)


fifo = probe.WideFifoAdapter(make_wide_sample(17), make_wide_sample(23))
assert isinstance(fifo, tlm.tlm_fifo_debug_if[sc_dt.sc_biguint[137]])
assert isinstance(fifo, tlm.tlm_fifo_config_size_if)
newest = sc_dt.sc_biguint[137](0)
oldest = sc_dt.sc_biguint[137](0)
assert fifo.nb_peek(newest, 0) and fifo.nb_peek(oldest, 1)
assert wide_record(newest) == (17, 1) and wide_record(oldest) == (23, 1), (
    wide_record(newest),
    wide_record(oldest),
)
assert fifo.nb_poke(make_wide_sample(31))
changed = sc_dt.sc_biguint[137](0)
assert fifo.nb_peek(changed, 0)
assert not fifo.nb_poke(sc_dt.sc_biguint[137](99), 2)
fifo.debug()
fifo.nb_expand()
after_expand = fifo.size()
fifo.nb_unbound()
after_unbound = fifo.size()
reduced = fifo.nb_reduce()
after_reduce = fifo.size()
small_bound = fifo.nb_bound(1)
valid_bound = fifo.nb_bound(2)
assert (reduced, small_bound, valid_bound) == (True, False, True)
assert fifo.used() == 2 and fifo.debug_calls() == 1
assert wide_record(changed) == (31, 1), wide_record(changed)
print(
    "TLM_IFACE_FIFO",
    fifo.used(),
    after_expand,
    after_unbound,
    after_reduce,
    int(valid_bound),
    fifo.debug_calls(),
    *wide_record(newest),
    *wide_record(oldest),
    *wide_record(changed),
)


base = probe.BaseProtocolAdapter()
protocol = tlm.tlm_base_protocol_types
assert isinstance(protocol(), protocol)
forward_interface = tlm.tlm_fw_transport_if[protocol]
backward_interface = tlm.tlm_bw_transport_if[protocol]
assert isinstance(base, forward_interface) and isinstance(base, backward_interface)
assert forward_interface.protocol_types == protocol
assert backward_interface.protocol_types == protocol
for interface in (
    tlm.tlm_blocking_transport_if[tlm.tlm_generic_payload],
    tlm.tlm_fw_nonblocking_transport_if[tlm.tlm_generic_payload, tlm.tlm_phase],
    tlm.tlm_bw_nonblocking_transport_if[tlm.tlm_generic_payload, tlm.tlm_phase],
    tlm.tlm_fw_direct_mem_if[tlm.tlm_generic_payload],
    tlm.tlm_bw_direct_mem_if,
    tlm.tlm_transport_dbg_if[tlm.tlm_generic_payload],
):
    assert isinstance(base, interface)
assert protocol.tlm_payload_type == tlm.tlm_generic_payload
assert protocol.tlm_phase_type == tlm.tlm_phase
payload = tlm.tlm_generic_payload()
phase = tlm.tlm_phase(tlm.BEGIN_REQ)
delay = sc.sc_time(2, sc.SC_NS)
base.b_transport(payload, delay)
forward = base.nb_transport_fw(payload, phase, delay)
backward = base.nb_transport_bw(payload, phase, delay)
dmi = tlm.tlm_dmi()
granted = base.get_direct_mem_ptr(payload, dmi)
base.invalidate_direct_mem_ptr(0x20, 0x2F)
transferred = base.transport_dbg(payload)
assert delay == sc.sc_time(3, sc.SC_NS)
assert forward == tlm.TLM_UPDATED and backward == tlm.TLM_COMPLETED
assert granted and int(dmi.get_start_address()) == 0x10 and int(dmi.get_end_address()) == 0x1F
assert transferred == 4
assert int(base.invalidated_start()) == 0x20 and int(base.invalidated_end()) == 0x2F
print(
    "TLM_IFACE_PROTOCOL",
    int(delay.value()),
    int(forward),
    int(backward),
    int(granted),
    int(dmi.get_start_address()),
    int(dmi.get_end_address()),
    int(transferred),
    base.fw_calls(),
    base.bw_calls(),
    base.blocking_calls(),
    base.dmi_calls(),
    base.invalidation_calls(),
    base.debug_transport_calls(),
)

assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
for adapter in (wide, writer, fifo, base):
    adapter.__python_owns__ = False
    probe.destroy_interface(adapter)
assert list(probe.destruction_counts()) == [1, 1, 1, 1]
print("TLM_IFACE_DESTRUCTION 1 1 1 1")
print("TLM_INTERFACES_PUBLIC_OK")
