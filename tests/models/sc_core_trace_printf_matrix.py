"""Exercise every supported formatted trace conversion and integer length."""

import ctypes
import gc
import locale
import weakref
from array import array

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.cppdef("""
namespace trace_printf_measurement_probe
{
/** @brief Verify the size-only pass cannot alter borrowed caller count storage.
 * @details The test owns its counter; the GIL stays held and no callbacks run.
 */
bool isolated()
{
  int caller = -17;
  pysystemc_public::trace_count_argument<int> count(&caller);
  int size = std::snprintf(nullptr, 0, "abc%n", pysystemc_public::trace_printf_measure_arg(count));
  return size == 3 && caller == -17 && count.measurement == 3;
}
}
""")
assert cppyy.gbl.trace_printf_measurement_probe.isolated()

locale.setlocale(locale.LC_ALL, "C")
CASES = [
    ("grouping_flag", "%'d", [1234567]),
    ("signed_default_d", "%d", [-17]),
    ("signed_default_i", "%i", [-17]),
    ("signed_hh_d", "%hhd", [-17]),
    ("signed_hh_i", "%hhi", [-17]),
    ("signed_h_d", "%hd", [-17]),
    ("signed_h_i", "%hi", [-17]),
    ("signed_l_d", "%ld", [-17]),
    ("signed_l_i", "%li", [-17]),
    ("signed_ll_d", "%lld", [-17]),
    ("signed_ll_i", "%lli", [-17]),
    ("signed_j_d", "%jd", [-17]),
    ("signed_j_i", "%ji", [-17]),
    ("signed_z_d", "%zd", [-17]),
    ("signed_z_i", "%zi", [-17]),
    ("signed_t_d", "%td", [-17]),
    ("signed_t_i", "%ti", [-17]),
    ("unsigned_default_u", "%#u", [255]),
    ("unsigned_default_o", "%#o", [255]),
    ("unsigned_default_x", "%#x", [255]),
    ("unsigned_default_X", "%#X", [255]),
    ("unsigned_hh_u", "%#hhu", [255]),
    ("unsigned_hh_o", "%#hho", [255]),
    ("unsigned_hh_x", "%#hhx", [255]),
    ("unsigned_hh_X", "%#hhX", [255]),
    ("unsigned_h_u", "%#hu", [255]),
    ("unsigned_h_o", "%#ho", [255]),
    ("unsigned_h_x", "%#hx", [255]),
    ("unsigned_h_X", "%#hX", [255]),
    ("unsigned_l_u", "%#lu", [255]),
    ("unsigned_l_o", "%#lo", [255]),
    ("unsigned_l_x", "%#lx", [255]),
    ("unsigned_l_X", "%#lX", [255]),
    ("unsigned_ll_u", "%#llu", [255]),
    ("unsigned_ll_o", "%#llo", [255]),
    ("unsigned_ll_x", "%#llx", [255]),
    ("unsigned_ll_X", "%#llX", [255]),
    ("unsigned_j_u", "%#ju", [255]),
    ("unsigned_j_o", "%#jo", [255]),
    ("unsigned_j_x", "%#jx", [255]),
    ("unsigned_j_X", "%#jX", [255]),
    ("unsigned_z_u", "%#zu", [255]),
    ("unsigned_z_o", "%#zo", [255]),
    ("unsigned_z_x", "%#zx", [255]),
    ("unsigned_z_X", "%#zX", [255]),
    ("unsigned_t_u", "%#tu", [255]),
    ("unsigned_t_o", "%#to", [255]),
    ("unsigned_t_x", "%#tx", [255]),
    ("unsigned_t_X", "%#tX", [255]),
    ("float_default_f", "%+.4f", [-1.25]),
    ("float_default_F", "%+.4F", [-1.25]),
    ("float_default_e", "%+.4e", [-1.25]),
    ("float_default_E", "%+.4E", [-1.25]),
    ("float_default_g", "%+.4g", [-1.25]),
    ("float_default_G", "%+.4G", [-1.25]),
    ("float_default_a", "%+.4a", [-1.25]),
    ("float_default_A", "%+.4A", [-1.25]),
    ("float_l_f", "%+.4lf", [-1.25]),
    ("float_l_F", "%+.4lF", [-1.25]),
    ("float_l_e", "%+.4le", [-1.25]),
    ("float_l_E", "%+.4lE", [-1.25]),
    ("float_l_g", "%+.4lg", [-1.25]),
    ("float_l_G", "%+.4lG", [-1.25]),
    ("float_l_a", "%+.4la", [-1.25]),
    ("float_l_A", "%+.4lA", [-1.25]),
    ("float_L_f", "%+.4Lf", [-1.25]),
    ("float_L_F", "%+.4LF", [-1.25]),
    ("float_L_e", "%+.4Le", [-1.25]),
    ("float_L_E", "%+.4LE", [-1.25]),
    ("float_L_g", "%+.4Lg", [-1.25]),
    ("float_L_G", "%+.4LG", [-1.25]),
    ("float_L_a", "%+.4La", [-1.25]),
    ("float_L_A", "%+.4LA", [-1.25]),
    ("star", "%*.*f", [-12, 4, 1.25]),
    ("star_negative_precision", "%.*g", [-1, 1.25]),
    ("precision_zero", "%.0d %.0o %.0x", [0, 0, 0]),
    ("integer_flags", "%+08d %#08X %-8u % d", [-17, 255, 29, 17]),
    ("strings", "%.3s %8s %-8ls", ["alphabet", "word", "wide"]),
    ("characters", "%c %lc", ["Q", "W"]),
    ("positional_reuse", "%2$i %1$s %2$08i", ["word", 17]),
    ("positional_star", "%3$*1$.*2$G", [10, 4, 1.25]),
    ("null_pointer", "%p", [None]),
    ("percent", "%%", []),
]
COUNT_TYPES = (("", "i"), ("hh", "b"), ("h", "h"), ("l", "l"), ("ll", "q"), ("j", "l"), ("z", "l"), ("t", "l"))
signal = sc.sc_signal[bool]("sample")
pending = []
for format_name, create, close in (
    ("vcd", sc.sc_create_vcd_trace_file, sc.sc_close_vcd_trace_file),
    ("wif", sc.sc_create_wif_trace_file, sc.sc_close_wif_trace_file),
):
    trace = create("printf_" + format_name)
    trace.set_time_unit(1, sc.SC_NS)
    sc.sc_trace(trace, signal, "sample")
    for label, conversion, arguments in CASES:
        sc.tprintf(trace, "PRINTF " + label + " " + conversion, *arguments)
    for modifier, typecode in COUNT_TYPES:
        label = "count_" + (modifier or "default")
        prefix = "PRINTF " + label + " abc"
        count = array(typecode, [-1])
        sc.tprintf(trace, prefix + "%" + modifier + "n", count)
        assert count[0] == len(prefix)
        sc.tprintf(trace, "PRINTF " + label + "_value %d", count[0])
    pending.append((trace, close))

sc.sc_start(1, sc.SC_NS)
for trace, close in pending:
    close(trace)

for modifier, counter_type in (
    ("", ctypes.c_int),
    ("hh", ctypes.c_byte),
    ("h", ctypes.c_short),
    ("l", ctypes.c_long),
    ("ll", ctypes.c_longlong),
    ("j", ctypes.c_int64),
    ("z", ctypes.c_ssize_t),
    ("t", ctypes.c_ssize_t),
):
    counter = (counter_type * 1)(-1)
    sc.tprintf(None, "native count%" + modifier + "n", counter)
    assert counter[0] == len("native count")
    pointer = ctypes.pointer(counter_type(-1))
    sc.tprintf(None, "pointer count%" + modifier + "n", pointer)
    assert pointer.contents.value == len("pointer count")
    referent = counter_type(-1)
    sc.tprintf(None, "reference count%" + modifier + "n", ctypes.byref(referent))
    assert referent.value == len("reference count")

transient = array("b", [-1])
owner_ref = weakref.ref(transient)
sc.tprintf(None, "abc%hhn", transient)
assert transient[0] == 3
del transient
gc.collect()
assert owner_ref() is None

INVALID = (
    ("%d", (), TypeError),
    ("text", (1,), TypeError),
    ("%n", (1,), TypeError),
    ("%n", (None,), TypeError),
    ("%n", (cppyy.nullptr,), TypeError),
    ("%n", (ctypes.POINTER(ctypes.c_int)(),), TypeError),
    ("%n", (ctypes.pointer(ctypes.c_float(0)),), TypeError),
    ("%n", (ctypes.byref(ctypes.c_float(0)),), TypeError),
    ("%n", (ctypes.byref(ctypes.c_int(0), ctypes.sizeof(ctypes.c_int)),), TypeError),
    ("%n", (sc.sc_time(1, sc.SC_NS),), TypeError),
    ("%+n", (array("i", [0]),), ValueError),
    ("%3n", (array("i", [0]),), ValueError),
    ("%.1n", (array("i", [0]),), ValueError),
    ("%lln", (array("l", [0]),), TypeError),
    ("%ln", (array("q", [0]),), TypeError),
    ("%n", ((ctypes.c_int * 1).from_buffer(bytearray(16), 1),), TypeError),
    ("%n", (ctypes.cast(ctypes.byref((ctypes.c_byte * 16)(), 1), ctypes.POINTER(ctypes.c_int)),), TypeError),
    ("%n", (array("f", [0]),), TypeError),
    ("%n", (array("i"),), TypeError),
    ("%n", (memoryview(array("i", [0, 0, 0]))[::2],), TypeError),
    ("%n", (memoryview(array("i", [0])).toreadonly(),), TypeError),
    ("%Ln", (array("i", [0]),), ValueError),
    ("%Ld", (1,), ValueError),
    ("%hs", ("word",), ValueError),
    ("%lp", (None,), ValueError),
    ("%2$d %d", (1, 2), ValueError),
    ("%1$*d", (1, 2), ValueError),
    ("%0$d", (1,), TypeError),
    ("%c", ("ab",), TypeError),
    ("%q", (1,), ValueError),
    ("%", (), ValueError),
    ("nul\0", (), ValueError),
    (1, (), ValueError),
)
for conversion, arguments, error_type in INVALID:
    try:
        sc.tprintf(None, conversion, *arguments)
    except error_type:
        pass
    else:
        raise AssertionError(f"accepted invalid format {conversion!r}")
print(f"PRINTF_MATRIX cases={len(CASES) + 2 * len(COUNT_TYPES)} formats=2 errors={len(INVALID)}")
