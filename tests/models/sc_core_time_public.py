"""Exercise every native time declaration and conversion in a fresh kernel."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy

sc.sc_set_time_resolution(1, sc.SC_PS)
sc.sc_set_default_time_unit(1, sc.SC_NS)
T = sc.sc_time
context = sc.sc_get_curr_simcontext()


def emit(label, value):
    """Record exact ticks while exercising every public scalar conversion."""
    stream = cppyy.gbl.std.ostringstream()
    value.print(stream)
    assert str(stream.str()) == str(value.to_string())
    print(
        "TIME_PUBLIC",
        label,
        int(value.value()),
        f"{value.to_double():.1f}",
        f"{value.to_seconds():.17g}",
        f"{value.to_default_time_units():.17g}",
        str(stream.str()),
    )


emit("default", T())
selected = T.__new__(T)
T.__init__.__overload__("sc_core::sc_time::value_type,bool")(selected, (1 << 63) + 17, False)
assert selected.value() == (1 << 63) + 17
base = T(1.25, sc.SC_NS)
assert T(1.25, unit="ns") == base
assert T((1 << 63) + 17, scale=False).value() == (1 << 63) + 17
emit("copy", T(base))
for unit, symbol in [
    (sc.SC_FS, "fs"),
    (sc.SC_PS, "ps"),
    (sc.SC_NS, "ns"),
    (sc.SC_US, "us"),
    (sc.SC_MS, "ms"),
    (sc.SC_SEC, "s"),
]:
    expected = T(2.5, unit)
    assert T(2.5, unit, context) == expected
    assert T(2.5, symbol) == expected
    assert T(2.5, symbol, context) == expected
    assert (
        T(2.5, "SC_" + str(symbol).upper().replace("S", "SEC") if symbol == "s" else "SC_" + symbol.upper()) == expected
    )
    emit(symbol, expected)
emit("seconds", T.from_seconds(1.25e-9))
emit("string", T.from_string(" 1.25 ns"))
emit("ticks", T.from_value((1 << 63) + 17))
assert cppyy.sizeof(T.value_type) == cppyy.sizeof(sc_dt.uint64) == 8
assert T.value_type(17) == 17
for scalar, label in [(2.5, "double"), (3, "uint64")]:
    for scale in (False, True):
        value = T(scalar, scale)
        emit(label + str(int(scale)), value)
assert T((1 << 63) + 17, False).value() == (1 << 63) + 17
copy = T()
assert cppyy.addressof(copy.__assign__(base)) == cppyy.addressof(copy)
assert cppyy.addressof(copy.__assign__(copy)) == cppyy.addressof(copy)
assert base.value() == 1250 and copy.value() == 1250
print(
    "TIME_RELATIONS",
    int(copy == base),
    int(copy != T()),
    int(T() < base),
    int(copy <= base),
    int(base > T()),
    int(copy >= base),
)
for label, operation, argument in [
    ("add", "__iadd__", T.from_value(251)),
    ("sub", "__isub__", T.from_value(251)),
    ("mul", "__imul__", 1.5),
    ("div", "__itruediv__", 1.5),
    ("mod", "__imod__", T.from_value(251)),
]:
    value = T(base)
    result = getattr(value, operation)(argument)
    assert cppyy.addressof(result) == cppyy.addressof(value)
    assert base.value() == 1250
    emit(label, value)
for text in ("", "-1 ns", "abc ns", "1", "1 bad"):
    try:
        T.from_string(text)
    except sc.sc_report as error:
        print("TIME_ERROR", "string", int(error.get_id()))
    else:
        raise AssertionError("invalid time string accepted")
for unit in ("", "invalid"):
    for explicit in (False, True):
        try:
            T(1.0, unit, context) if explicit else T(1.0, unit)
        except sc.sc_report as error:
            print("TIME_ERROR", "unit", int(error.get_id()))
        else:
            raise AssertionError("invalid time unit accepted")
assert T.from_string("3 ns").value() == 3000
print("TIME_RECOVERY", 3000)
