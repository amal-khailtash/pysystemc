"""Exercise direct Python dispatch through every sc_trace_file virtual overload."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_trace_file_probe.h"))
probe_type = cppyy.gbl.sc_core.TraceFileProbe

try:
    sc.sc_trace_file()
except TypeError:
    pass
else:
    raise AssertionError("abstract sc_trace_file construction succeeded")

trace = probe_type()
assert trace.records().empty()


trace.trace(trace.event_sample(), "event")
trace.trace(trace.time_sample(), "time")
trace.trace(sc.sc_trace_value["bool"](trace.bool_sample()), "bool")
trace.trace(trace.bit_sample(), "bit")
trace.trace(trace.logic_sample(), "logic")
trace.trace(sc.sc_trace_value["unsigned char"](trace.uchar_sample()), "uchar", 5)
trace.trace(sc.sc_trace_value["unsigned short"](trace.ushort_sample()), "ushort", 9)
trace.trace(sc.sc_trace_value["unsigned int"](trace.uint_sample()), "uint", 11)
trace.trace(sc.sc_trace_value["unsigned long"](trace.ulong_sample()), "ulong", 13)
trace.trace(sc.sc_trace_value["char"](trace.char_sample()), "char", 6)
trace.trace(sc.sc_trace_value["short"](trace.short_sample()), "short", 10)
trace.trace(sc.sc_trace_value["int"](trace.int_sample()), "int", 12)
trace.trace(sc.sc_trace_value["long"](trace.long_sample()), "long", 14)
trace.trace(sc.sc_trace_value["sc_dt::int64"](trace.int64_sample()), "int64", 17)
trace.trace(sc.sc_trace_value["sc_dt::uint64"](trace.uint64_sample()), "uint64", 19)
trace.trace(sc.sc_trace_value["float"](trace.float_sample()), "float")
trace.trace(sc.sc_trace_value["double"](trace.double_sample()), "double")
trace.trace(trace.sc_int_sample(), "sc_int")
trace.trace(trace.sc_uint_sample(), "sc_uint")
# Verify that explicit native overload selection remains available on a
# cppyy-native value, alongside the holder-aware familiar trace signature.
trace.trace.__overload__("const sc_dt::sc_signed&,const std::string&")(trace, trace.signed_wide_sample(), "signed")
trace.trace(trace.unsigned_wide_sample(), "unsigned")
trace.trace(trace.fxval_sample(), "fxval")
trace.trace(trace.fxval_fast_sample(), "fxval_fast")
trace.trace(trace.fixed_sample(), "fxnum")
trace.trace(trace.fixed_fast_sample(), "fxnum_fast")
trace.trace(trace.bits_sample(), "bv")
trace.trace(trace.logic_vector_sample(), "lv")
trace.trace(sc.sc_trace_value["unsigned int"](trace.uint_sample()), "enum", ["IDLE", "BUSY", None])
assert len(trace._trace_value_owners) == 15

trace.write_comment("native comment")
trace.space(2)
trace.delta_cycles(True)
trace.delta_cycles(False)
trace.set_time_unit(0.5, sc.SC_PS)
trace.invoke_cycle(True)
trace.invoke_cycle(False)

records = trace.records()
assert records.size() == 34, records.size()
for index in range(records.size()):
    print(f"TRACE_FILE|{records[index]}")

del records
del trace
gc.collect()
assert probe_type.destruction_count() == 1, probe_type.destruction_count()
print(f"TRACE_FILE_DESTRUCTIONS|{probe_type.destruction_count()}")
