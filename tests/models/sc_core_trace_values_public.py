"""Trace mutable native value holders through GC and exact VCD closure."""

import gc
import weakref

from amal.eda.systemc import sc_core as sc, sc_dt

trace = sc.sc_create_vcd_trace_file("trace_values")
trace.set_time_unit(1, sc.SC_PS)
values = {
    "float_value": sc.sc_trace_value["float"](1.5),
    "double_value": sc.sc_trace_value["double"](2.25),
    "short_value": sc.sc_trace_value["unsigned short"](3),
    "wide_integer": sc.sc_trace_value["sc_dt::uint64"]((1 << 63) + 5),
    "boolean": sc.sc_trace_value["bool"](False),
    "state": sc.sc_trace_value["unsigned int"](0),
    "wide_vector": sc.sc_trace_value[sc_dt.sc_biguint[137]](sc_dt.sc_biguint[137](17)),
}
trace.trace(values["float_value"], "float_value")
trace.trace(values["double_value"], "double_value")
trace.trace(values["short_value"], "short_value", 9)
sc.sc_trace(trace, values["wide_integer"], "wide_integer", 64)
trace.trace(values["boolean"], "boolean")
trace.trace(values["state"], "state", ["idle", "running", "done", None])
sc.sc_trace(trace, values["wide_vector"], "wide_vector")
sc.sc_trace(None, values["float_value"], "ignored")
refs = {name: weakref.ref(value) for name, value in values.items()}
del values
gc.collect()
assert all(ref() is not None for ref in refs.values())
assert len(trace._trace_value_owners) == 8
sc.sc_start(1, sc.SC_NS)
refs["float_value"]().value = 2.75
refs["double_value"]().value = -3.5
refs["short_value"]().value = 319
refs["wide_integer"]().value = (1 << 63) + 17
refs["boolean"]().value = True
refs["state"]().value = 1
refs["wide_vector"]().value.__assign__(sc_dt.sc_biguint[137](str((1 << 136) + 257)))
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
sc.sc_close_vcd_trace_file(trace)
gc.collect()
assert all(ref() is None for ref in refs.values())
assert "_trace_value_owners" not in trace.__dict__
print("TRACE_VALUES_LIFETIME 7 8 0")
print("TRACE_VALUES_TIME", int(sc.sc_time_stamp().value()))
