"""Verify public time tuple normalization and conversion against native records."""

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef("""
namespace time_tuple_probe {
/** Convert a borrowed tuple with native operator sc_time; GIL held, no scheduler entry. */
sc_core::sc_time convert(const sc_core::sc_time_tuple& tuple) { return tuple; }
}
""")
for index, time in enumerate(
    (
        sc.SC_ZERO_TIME,
        sc.sc_time(1, sc.SC_FS),
        sc.sc_time(1500, sc.SC_PS),
        sc.sc_time(2500, sc.SC_NS),
        sc.sc_time(3, sc.SC_SEC),
    )
):
    tuple_ = sc.sc_time_tuple() if index == 0 else sc.sc_time_tuple(time)
    assert cppyy.gbl.time_tuple_probe.convert(tuple_) == time
    assert tuple_.has_value()
    assert tuple_.to_double() >= 0
    print(
        "TIME_TUPLE_REFERENCE",
        index,
        int(tuple_.has_value()),
        tuple_.value(),
        int(tuple_.unit()),
        tuple_.unit_symbol(),
        format(tuple_.to_double(), ".17g"),
        tuple_.to_string(),
    )
