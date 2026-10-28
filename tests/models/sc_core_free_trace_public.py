"""Verify every free trace overload, widths and delayed port registration."""

from pathlib import Path
import ctypes
import gc
import weakref

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_free_trace_probe.h"))
probe = cppyy.gbl.free_trace_probe
file = cppyy.gbl.sc_core.TraceFileProbe()
sc.sc_trace(file, file.event_sample(), "event_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_core::sc_event*,const std::string&")(
    file, probe.event_pointer(file), "event_ptr"
)
sc.sc_trace(file, file.time_sample(), "time_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_core::sc_time*,const std::string&")(
    file, probe.time_pointer(file), "time_ptr"
)
sc.sc_trace(file, sc.sc_trace_value["bool"](file.bool_sample()), "bool_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const bool*,const std::string&")(
    file, probe.bool_pointer(file), "bool_ptr"
)
sc.sc_trace(file, sc.sc_trace_value["float"](file.float_sample()), "float_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const float*,const std::string&")(
    file, probe.float_pointer(file), "float_ptr"
)
sc.sc_trace(file, sc.sc_trace_value["double"](file.double_sample()), "double_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const double*,const std::string&")(
    file, probe.double_pointer(file), "double_ptr"
)
sc.sc_trace(file, file.bit_sample(), "bit_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_bit*,const std::string&")(
    file, probe.bit_pointer(file), "bit_ptr"
)
sc.sc_trace(file, file.logic_sample(), "logic_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_logic*,const std::string&")(
    file, probe.logic_pointer(file), "logic_ptr"
)
sc.sc_trace(file, file.sc_int_sample(), "sc_int_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_int_base*,const std::string&")(
    file, probe.sc_int_pointer(file), "sc_int_ptr"
)
sc.sc_trace(file, file.sc_uint_sample(), "sc_uint_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_uint_base*,const std::string&")(
    file, probe.sc_uint_pointer(file), "sc_uint_ptr"
)
sc.sc_trace(file, file.signed_wide_sample(), "signed_wide_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_signed*,const std::string&")(
    file, probe.signed_wide_pointer(file), "signed_wide_ptr"
)
sc.sc_trace(file, file.unsigned_wide_sample(), "unsigned_wide_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_unsigned*,const std::string&")(
    file, probe.unsigned_wide_pointer(file), "unsigned_wide_ptr"
)
sc.sc_trace(file, file.bits_sample(), "bits_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_bv_base*,const std::string&")(
    file, probe.bits_pointer(file), "bits_ptr"
)
sc.sc_trace(file, file.logic_vector_sample(), "logic_vector_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_lv_base*,const std::string&")(
    file, probe.logic_vector_pointer(file), "logic_vector_ptr"
)
sc.sc_trace(file, file.fxval_sample(), "fxval_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_fxval*,const std::string&")(
    file, probe.fxval_pointer(file), "fxval_ptr"
)
sc.sc_trace(file, file.fxval_fast_sample(), "fxval_fast_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_fxval_fast*,const std::string&")(
    file, probe.fxval_fast_pointer(file), "fxval_fast_ptr"
)
sc.sc_trace(file, file.fixed_sample(), "fixed_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_fxnum*,const std::string&")(
    file, probe.fixed_pointer(file), "fixed_ptr"
)
sc.sc_trace(file, file.fixed_fast_sample(), "fixed_fast_ref")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::sc_fxnum_fast*,const std::string&")(
    file, probe.fixed_fast_pointer(file), "fixed_fast_ptr"
)
sc.sc_trace(file, sc.sc_trace_value["unsigned char"](file.uchar_sample()), "uchar_ref", 7)
sc.sc_trace(file, sc.sc_trace_value["unsigned char"](file.uchar_sample()), "uchar_ref_default")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const unsigned char*,const std::string&,int")(
    file, probe.uchar_pointer(file), "uchar_ptr", 9
)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const unsigned char*,const std::string&,int")(
    file, probe.uchar_pointer(file), "uchar_ptr_default"
)
sc.sc_trace(file, sc.sc_trace_value["unsigned short"](file.ushort_sample()), "ushort_ref", 8)
sc.sc_trace(file, sc.sc_trace_value["unsigned short"](file.ushort_sample()), "ushort_ref_default")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const unsigned short*,const std::string&,int")(
    file, probe.ushort_pointer(file), "ushort_ptr", 10
)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const unsigned short*,const std::string&,int")(
    file, probe.ushort_pointer(file), "ushort_ptr_default"
)
sc.sc_trace(file, sc.sc_trace_value["unsigned int"](file.uint_sample()), "uint_ref", 9)
sc.sc_trace(file, sc.sc_trace_value["unsigned int"](file.uint_sample()), "uint_ref_default")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const unsigned int*,const std::string&,int")(
    file, probe.uint_pointer(file), "uint_ptr", 11
)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const unsigned int*,const std::string&,int")(
    file, probe.uint_pointer(file), "uint_ptr_default"
)
sc.sc_trace(file, sc.sc_trace_value["unsigned long"](file.ulong_sample()), "ulong_ref", 10)
sc.sc_trace(file, sc.sc_trace_value["unsigned long"](file.ulong_sample()), "ulong_ref_default")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const unsigned long*,const std::string&,int")(
    file, probe.ulong_pointer(file), "ulong_ptr", 12
)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const unsigned long*,const std::string&,int")(
    file, probe.ulong_pointer(file), "ulong_ptr_default"
)
sc.sc_trace(file, sc.sc_trace_value["char"](file.char_sample()), "char_ref", 11)
sc.sc_trace(file, sc.sc_trace_value["char"](file.char_sample()), "char_ref_default")
# cppyy decodes const char* as text, so retain a typed alias at the native address.
char_pointer_address = probe.char_pointer_address(file)
char_storage = (ctypes.c_char * 1).from_address(char_pointer_address)
assert ctypes.addressof(char_storage) == char_pointer_address
char_pointer = ctypes.cast(char_storage, ctypes.POINTER(ctypes.c_char))
assert ctypes.cast(char_pointer, ctypes.c_void_p).value == char_pointer_address
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const char*,const std::string&,int")(
    file, char_pointer, "char_ptr", 13
)
sc.sc_trace(file, char_storage, "char_ptr_default")
null_char_pointer = ctypes.POINTER(ctypes.c_char)()
for call in (
    lambda: sc.sc_trace(file, null_char_pointer, "null_char"),
    lambda: sc.sc_trace.__overload__("sc_core::sc_trace_file*,const char*,const std::string&,int")(
        file, null_char_pointer, "null_char_selected"
    ),
):
    try:
        call()
    except ValueError:
        pass
    else:
        raise AssertionError("null char pointer was accepted for a live trace file")
try:
    sc.sc_trace(file, (ctypes.c_char * 0)(), "empty_char_array")
except TypeError:
    pass
else:
    raise AssertionError("empty char array was accepted for tracing")
sc.sc_trace(file, sc.sc_trace_value["short"](file.short_sample()), "short_ref", 12)
sc.sc_trace(file, sc.sc_trace_value["short"](file.short_sample()), "short_ref_default")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const short*,const std::string&,int")(
    file, probe.short_pointer(file), "short_ptr", 14
)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const short*,const std::string&,int")(
    file, probe.short_pointer(file), "short_ptr_default"
)
sc.sc_trace(file, sc.sc_trace_value["int"](file.int_sample()), "int_ref", 13)
sc.sc_trace(file, sc.sc_trace_value["int"](file.int_sample()), "int_ref_default")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const int*,const std::string&,int")(
    file, probe.int_pointer(file), "int_ptr", 15
)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const int*,const std::string&,int")(
    file, probe.int_pointer(file), "int_ptr_default"
)
sc.sc_trace(file, sc.sc_trace_value["long"](file.long_sample()), "long_ref", 14)
sc.sc_trace(file, sc.sc_trace_value["long"](file.long_sample()), "long_ref_default")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const long*,const std::string&,int")(
    file, probe.long_pointer(file), "long_ptr", 16
)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const long*,const std::string&,int")(
    file, probe.long_pointer(file), "long_ptr_default"
)
sc.sc_trace(file, sc.sc_trace_value["sc_dt::int64"](file.int64_sample()), "int64_ref", 15)
sc.sc_trace(file, sc.sc_trace_value["sc_dt::int64"](file.int64_sample()), "int64_ref_default")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::int64*,const std::string&,int")(
    file, probe.int64_pointer(file), "int64_ptr", 17
)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::int64*,const std::string&,int")(
    file, probe.int64_pointer(file), "int64_ptr_default"
)
sc.sc_trace(file, sc.sc_trace_value["sc_dt::uint64"](file.uint64_sample()), "uint64_ref", 16)
sc.sc_trace(file, sc.sc_trace_value["sc_dt::uint64"](file.uint64_sample()), "uint64_ref_default")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::uint64*,const std::string&,int")(
    file, probe.uint64_pointer(file), "uint64_ptr", 18
)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_dt::uint64*,const std::string&,int")(
    file, probe.uint64_pointer(file), "uint64_ptr_default"
)
sc.sc_trace(file, sc.sc_trace_value["unsigned int"](1), "enum", ["IDLE", "BUSY", None])
warning_count = sc.sc_report_handler.get_count(sc.SC_WARNING)
sc.sc_report_handler.set_actions(sc.SC_WARNING, sc.SC_DO_NOTHING)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const void*,const std::string&")(file, cppyy.nullptr, "ignored")
assert sc.sc_report_handler.get_count(sc.SC_WARNING) == warning_count + 1
print("FREE_TRACE_IGNORED 1")
sc.sc_report_handler.set_actions(sc.SC_WARNING, sc.SC_DISPLAY)
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const float*,const std::string&")(
    cppyy.nullptr, cppyy.nullptr, "null"
)
signal_char = sc.sc_signal["char"]("signal_char", 5)
sc.sc_trace(file, signal_char, "signal_char", 6)
signal_short = sc.sc_signal["short"]("signal_short", 5)
sc.sc_trace(file, signal_short, "signal_short", 7)
signal_int = sc.sc_signal["int"]("signal_int", 5)
sc.sc_trace(file, signal_int, "signal_int", 8)
signal_long = sc.sc_signal["long"]("signal_long", 5)
sc.sc_trace(file, signal_long, "signal_long", 9)
floating = sc.sc_signal["float"]("floating", 1.25)
sc.sc_trace(file, floating, "floating_string")
try:
    sc.sc_trace(file, floating, "invalid_float_width", 7)
except TypeError:
    pass
else:
    raise AssertionError("floating signal accepted a nonnative width overload")
sc.sc_trace.__overload__("sc_core::sc_trace_file*,const sc_core::sc_signal_in_if<float>&,const char*")(
    file, floating, "floating_chars"
)
for row in file.records():
    print("FREE_TRACE|" + str(row))
wide_type = dt.sc_biguint[137]
integer = sc.sc_signal["int"]("integer", 3)
wide = sc.sc_signal[wide_type]("wide", wide_type(17))
ports = probe.ports("ports")
wave = sc.sc_create_vcd_trace_file("free_trace")
wave.set_time_unit(1, sc.SC_PS)
sc.sc_trace(wave, ports.input, "input_before_bind")
sc.sc_trace(wave, ports.inout, "inout_before_bind")
sc.sc_trace(wave, ports.wide_input, "wide_input_before_bind")
sc.sc_trace(wave, ports.wide_inout, "wide_inout_before_bind")
char_pointer_selector = sc.sc_trace.__overload__("sc_core::sc_trace_file*,const char*,const std::string&,int", False)
char_pointer_selector(wave, char_pointer, "char_pointer_wave")
sc.sc_trace(wave, char_storage, "char_array_wave")
char_pointer_ref = weakref.ref(char_pointer)
char_storage_ref = weakref.ref(char_storage)
ports.input.bind(integer)
ports.inout.bind(integer)
ports.wide_input.bind(wide)
ports.wide_inout.bind(wide)
sc.sc_trace(wave, integer, "integer_width", 7)
sc.sc_trace(wave, wide, "wide_generic")
sc.sc_start(1, sc.SC_NS)
assert sc.sc_port_value(ports.input) == integer.read()
assert sc.sc_port_value(ports.inout) == integer.read()
for port in (ports.wide_input, ports.wide_inout):
    value = sc.sc_port_value(port)
    assert value == wide.read()
    assert cppyy.addressof(value) == cppyy.addressof(wide.read())
char_pointer[0] = b"\xff"
del char_pointer, char_storage
gc.collect()
assert char_pointer_ref() is not None and char_storage_ref() is not None
sc.sc_trace(file, ports.input, "input_after_elaboration")
sc.sc_trace(file, ports.inout, "inout_after_elaboration")
for row in list(file.records())[81:]:
    print("FREE_TRACE|" + str(row))
integer.write(35)
wide.write(wide_type(str((1 << 136) + 257)))
sc.sc_start(1, sc.SC_NS)
sc.sc_close_vcd_trace_file(wave)
gc.collect()
assert char_pointer_ref() is None and char_storage_ref() is None
print("FREE_TRACE_TIME", int(sc.sc_time_stamp().value()))
