"""Verify public time tuple normalization and conversion against native records."""

from amal.eda.systemc import sc_core as sc
import cppyy
import sys

cppyy.cppdef("""
namespace time_tuple_probe {
/** Convert a borrowed tuple with native operator sc_time; GIL held, no scheduler entry. */
sc_core::sc_time convert(const sc_core::sc_time_tuple& tuple) { return tuple; }
}
""")
if len(sys.argv) > 1:
    sc.sc_set_time_resolution(10, sc.SC_FS)
    tuple_ = sc.sc_time_tuple(sc.sc_time.from_value(2**64 - 1))
    assert not tuple_.has_value()
    try:
        tuple_.value()
    except sc.sc_report as report:
        assert "sc_time_tuple value overflow" in report.get_msg()
    else:
        raise AssertionError("tuple overflow did not report")
    safe = sc.sc_time_tuple(sc.sc_time.from_value(1))
    assert safe.has_value() and safe.value() == 10
    print("TIME_TUPLE_OVERFLOW_REFERENCE", int(tuple_.has_value()), safe.value())
    sys.exit(0)

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
