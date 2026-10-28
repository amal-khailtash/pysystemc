"""Exercise arbitrary pointer and wide-value analysis FIFO specializations."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy


NATIVE = Path(__file__).resolve().parents[1] / "native"
pointer_fifo_type = tlm.tlm_analysis_fifo["int*"]
pointer_triple_type = tlm.tlm_analysis_triple["int*"]
wide_type = sc_dt.sc_biguint[191]
wide_fifo_type = tlm.tlm_analysis_fifo[wide_type]
wide_triple_type = tlm.tlm_analysis_triple[wide_type]
assert tlm.tlm_analysis_fifo["int*"] is pointer_fifo_type
assert tlm.tlm_analysis_fifo[wide_type] is wide_fifo_type

cppyy.include(str(NATIVE / "tlm_analysis_values_probe.h"))
probe = cppyy.gbl.tlm_analysis_values_probe
owner = probe.Owner(sc.sc_module_name("analysis_values_owner"))

# Pointer payloads are copied as addresses, so changing the owner vector after
# the writes must be observable when the FIFO returns those same pointers.
first_pointer = owner.pointer_at(0)
second_pointer = owner.pointer_at(2)
owner.pointer_fifo.write(first_pointer)
owner.pointer_fifo.write(pointer_triple_type(second_pointer))
owner.pointer_values[0] = 111
owner.pointer_values[1] = 222
owner.pointer_values[2] = 333
owner.pointer_values[3] = 444

# Wide value payloads are copied into native FIFO storage, including the high
# bit that cannot be represented in a built-in integer type.
first_wide = wide_type(165)
first_wide[190] = True
second_wide = wide_type(90)
second_wide[190] = True
owner.wide_fifo.write(first_wide)
owner.wide_fifo.write(wide_triple_type(second_wide))
first_wide = wide_type(101)
second_wide = wide_type(202)

sc.sc_start(sc.sc_time(0, sc.SC_NS))

first_pointer_read = probe.pop_pointer(owner.pointer_fifo)
second_pointer_read = probe.pop_pointer(owner.pointer_fifo)
assert first_pointer_read.address == probe.Owner.pointer_address(first_pointer)
assert second_pointer_read.address == probe.Owner.pointer_address(second_pointer)
assert (first_pointer_read.first, first_pointer_read.second) == (111, 222)
assert (second_pointer_read.first, second_pointer_read.second) == (333, 444)
print(
    "ANALYSIS_VALUES_POINTER",
    int(first_pointer_read.address == probe.Owner.pointer_address(first_pointer)),
    first_pointer_read.first,
    first_pointer_read.second,
    int(second_pointer_read.address == probe.Owner.pointer_address(second_pointer)),
    second_pointer_read.first,
    second_pointer_read.second,
)

first_wide_read = probe.pop_value[wide_type](owner.wide_fifo)
second_wide_read = probe.pop_value[wide_type](owner.wide_fifo)
assert first_wide_read[190].to_bool() and first_wide_read.to_uint64() == 165
assert second_wide_read[190].to_bool() and second_wide_read.to_uint64() == 90
print(
    "ANALYSIS_VALUES_WIDE",
    int(first_wide_read[190].to_bool()),
    first_wide_read.to_uint64(),
    int(second_wide_read[190].to_bool()),
    second_wide_read.to_uint64(),
)
print("TLM_ANALYSIS_VALUES_PUBLIC_OK")
