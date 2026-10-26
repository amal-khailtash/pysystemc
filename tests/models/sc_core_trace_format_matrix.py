"""Emit typed VCD or WIF traces from long-lived native sample storage.

The ``format`` argument selects one waveform backend per process because the
SystemC kernel is a singleton. Unsupported WIF time/event traces are attempted
under a local report action and checked as native ignored-object reports.
"""

import sys

from amal.eda.systemc import sc_core as sc
import cppyy


cppyy.cppdef(
    r"""
    #ifndef SC_INCLUDE_FX
    #define SC_INCLUDE_FX
    #endif
    #include <systemc>
    namespace trace_format_matrix {
    using namespace sc_core;
    using namespace sc_dt;
    /** @brief Own every mutable native sample used by the format matrix.
     * @details The Python test retains this object until matching trace closure,
     * so native files can safely borrow field addresses. It owns no callbacks,
     * makes no GIL transitions, and native reports/exceptions propagate.
     */
    struct Samples {
      bool boolean = false;
      sc_bit bit{false};
      sc_logic logic = SC_LOGIC_0;
      char schar = -3;
      unsigned char uchar = 3;
      short sshort = -5;
      unsigned short ushort = 5;
      int sint = -7;
      unsigned int uint = 7;
      long slong = -9;
      unsigned long ulong = 9;
      int64 int64_value = -11;
      uint64 uint64_value = (uint64(1) << 48) + 13;
      float single = 1.25f;
      double real = 2.25;
      sc_int<19> signed_fixed_width = -17;
      sc_uint<21> unsigned_fixed_width = 19;
      sc_signed signed_arbitrary{73};
      sc_unsigned unsigned_arbitrary{73};
      sc_bv<73> bits{"0000000000000000000000000000000000000000000000000000000000000"
                     "000000000001"};
      sc_lv<73> logic_vector{"00000000000000000000000000000000000000000000000000000"
                             "000000000000000000X1"};
      sc_fxval fx_value{1.25};
      sc_fxval_fast fx_fast_value{2.5};
      sc_fixed<16, 8> fixed_value{1.25};
      sc_fixed_fast<16, 8> fixed_fast_value{2.5};
      unsigned int enumeration = 0;
      sc_time time_value{3, SC_NS};
      sc_event event_value{"matrix_event"};
      /** @brief Construct initialized dynamic-width native integer storage. */
      Samples() {
        signed_arbitrary = -17;
        unsigned_arbitrary = 17;
      }
      /** @brief Change every waveform sample while retaining member addresses.
       * @param phase One of the two timed stimulus phases, 1 or 2.
       * @details Called synchronously with the GIL held from Python. No callback
       * is retained or invoked; a native report or exception propagates.
       */
      void advance(int phase) {
        const int signed_byte = phase == 1 ? -13 : -21;
        const int unsigned_byte = phase == 1 ? 33 : 65;
        const int signed_word = phase == 1 ? -29 : 31;
        const int unsigned_word = phase == 1 ? 129 : 257;
        const double floating = phase == 1 ? 2.5 : -3.75;
        const double real_value = phase == 1 ? -1.5 : 4.5;
        boolean = phase == 2;
        bit = sc_bit(phase == 1);
        logic = phase == 1 ? SC_LOGIC_1 : SC_LOGIC_Z;
        schar = static_cast<char>(signed_byte);
        uchar = static_cast<unsigned char>(unsigned_byte);
        sshort = static_cast<short>(signed_word);
        ushort = static_cast<unsigned short>(unsigned_word);
        sint = signed_byte * 100;
        uint = static_cast<unsigned int>(unsigned_byte * 100);
        slong = signed_word * 100;
        ulong = static_cast<unsigned long>(unsigned_word * 100);
        int64_value = signed_byte * 1000;
        uint64_value = (uint64(1) << 48) + unsigned_word * 1000;
        single = static_cast<float>(floating);
        real = real_value;
        signed_fixed_width = signed_byte;
        unsigned_fixed_width = unsigned_word;
        signed_arbitrary = signed_word;
        unsigned_arbitrary = unsigned_word;
        bits = phase == 1 ? "100000000000000000000000000000000000000000000000000000"
                            "0000000000000000001"
                          : "100000000000000000000000000000000000000000000000000000"
                            "0000000000000000011";
        logic_vector = phase == 1 ? "1000000000000000000000000000000000000000000000"
                                    "000000000000000000000000X1"
                                  : "1000000000000000000000000000000000000000000000"
                                    "000000000000000000000000Z0";
        fx_value = floating;
        fx_fast_value = real_value;
        fixed_value = floating;
        fixed_fast_value = real_value;
        enumeration = static_cast<unsigned int>(phase);
        time_value = sc_time(phase == 1 ? 5 : 8, SC_NS);
        event_value.notify(SC_ZERO_TIME);
      }
    };
    /** @brief Generate one typed borrowed-pointer getter for a Samples field.
     * @details Generated functions return storage owned by Samples and remain
     * valid only while that object lives. Calls hold the GIL; no callbacks or
     * native threads are involved and native exceptions propagate.
     */
    #define TRACE_MATRIX_POINTER(NAME, TYPE, FIELD)                                \
      inline const TYPE *NAME(Samples &samples) { return &samples.FIELD; }
    TRACE_MATRIX_POINTER(boolean_pointer, bool, boolean)
    TRACE_MATRIX_POINTER(bit_pointer, sc_bit, bit)
    TRACE_MATRIX_POINTER(logic_pointer, sc_logic, logic)
    TRACE_MATRIX_POINTER(sshort_pointer, short, sshort)
    TRACE_MATRIX_POINTER(ushort_pointer, unsigned short, ushort)
    TRACE_MATRIX_POINTER(sint_pointer, int, sint)
    TRACE_MATRIX_POINTER(uint_pointer, unsigned int, uint)
    TRACE_MATRIX_POINTER(slong_pointer, long, slong)
    TRACE_MATRIX_POINTER(ulong_pointer, unsigned long, ulong)
    TRACE_MATRIX_POINTER(int64_pointer, int64, int64_value)
    TRACE_MATRIX_POINTER(uint64_pointer, uint64, uint64_value)
    TRACE_MATRIX_POINTER(float_pointer, float, single)
    TRACE_MATRIX_POINTER(double_pointer, double, real)
    TRACE_MATRIX_POINTER(sc_int_pointer, sc_int_base, signed_fixed_width)
    TRACE_MATRIX_POINTER(sc_uint_pointer, sc_uint_base, unsigned_fixed_width)
    TRACE_MATRIX_POINTER(sc_signed_pointer, sc_signed, signed_arbitrary)
    TRACE_MATRIX_POINTER(sc_unsigned_pointer, sc_unsigned, unsigned_arbitrary)
    TRACE_MATRIX_POINTER(sc_bv_pointer, sc_bv_base, bits)
    TRACE_MATRIX_POINTER(sc_lv_pointer, sc_lv_base, logic_vector)
    TRACE_MATRIX_POINTER(fxval_pointer, sc_fxval, fx_value)
    TRACE_MATRIX_POINTER(fxval_fast_pointer, sc_fxval_fast, fx_fast_value)
    TRACE_MATRIX_POINTER(fxnum_pointer, sc_fxnum, fixed_value)
    TRACE_MATRIX_POINTER(fxnum_fast_pointer, sc_fxnum_fast, fixed_fast_value)
    TRACE_MATRIX_POINTER(time_pointer, sc_time, time_value)
    TRACE_MATRIX_POINTER(event_pointer, sc_event, event_value)
    #undef TRACE_MATRIX_POINTER
    } // namespace trace_format_matrix
    """
)


def assign_phase(samples, phase):
    """Change every declared value through stable C++ member storage.

    Args:
        samples: Native ``Samples`` object retained until trace closure.
        phase: One of the two strictly increasing timed stimulus phases.

    Raises:
        ValueError: If ``phase`` is not 1 or 2.
    """
    if phase not in (1, 2):
        raise ValueError("phase must be 1 or 2")
    samples.advance(phase)


def register_trace(trace, samples, fmt):
    """Register all format-applicable native values with stable lifetimes.

    Args:
        trace: Open VCD or WIF file created in the current process.
        samples: C++ holder object kept alive through file closure.
        fmt: ``"vcd"`` or ``"wif"``.
    """
    native = cppyy.gbl.trace_format_matrix
    width_values = (
        (native.sshort_pointer(samples), "sshort", 13),
        (native.ushort_pointer(samples), "ushort", 13),
        (native.sint_pointer(samples), "sint", 21),
        (native.uint_pointer(samples), "uint", 21),
        (native.slong_pointer(samples), "slong", 33),
        (native.ulong_pointer(samples), "ulong", 33),
        (native.int64_pointer(samples), "int64", 47),
        (native.uint64_pointer(samples), "uint64", 57),
    )
    sc.sc_trace(trace, native.boolean_pointer(samples), "boolean")
    sc.sc_trace(trace, native.bit_pointer(samples), "bit")
    sc.sc_trace(trace, native.logic_pointer(samples), "logic")
    # cppyy's signed-char conversion cannot display negative values, so avoid
    # reading the holder back. The native value and later bytes assignments
    # keep the signed-char storage exact while sc.sc_trace owns the holder.
    signed_char = sc.sc_trace_value["char"](bytes([253]))
    unsigned_char = sc.sc_trace_value["unsigned char"](3)
    sc.sc_trace(trace, signed_char, "schar", 8)
    sc.sc_trace(trace, unsigned_char, "uchar", 8)
    for value, name, width in width_values:
        sc.sc_trace(trace, value, name, width)
    for value, name in (
        (native.float_pointer(samples), "float"),
        (native.double_pointer(samples), "double"),
        (native.sc_int_pointer(samples), "sc_int19"),
        (native.sc_uint_pointer(samples), "sc_uint21"),
        (native.sc_signed_pointer(samples), "sc_signed73"),
        (native.sc_unsigned_pointer(samples), "sc_unsigned73"),
        (native.sc_bv_pointer(samples), "sc_bv73"),
        (native.sc_lv_pointer(samples), "sc_lv73"),
        (native.fxval_pointer(samples), "fxval"),
        (native.fxval_fast_pointer(samples), "fxval_fast"),
        (native.fxnum_pointer(samples), "fxnum"),
        (native.fxnum_fast_pointer(samples), "fxnum_fast"),
    ):
        sc.sc_trace(trace, value, name)
    enumeration = sc.sc_trace_value["unsigned int"](0)
    sc.sc_trace(trace, enumeration, "enumeration", ["IDLE", "BUSY", "DONE", None])

    if fmt == "vcd":
        sc.sc_trace(trace, native.time_pointer(samples), "time")
        sc.sc_trace(trace, native.event_pointer(samples), "event")
    else:
        # SystemC 2.3.4 emits SC_ID_TRACING_OBJECT_IGNORED_ errors for both
        # overloads. Suppress their default throw action only while checking
        # those native reports; the WIF backend must emit no such declarations.
        report_id = sc.SC_ID_TRACING_OBJECT_IGNORED_
        old_actions = sc.sc_report_handler.set_actions(report_id, sc.SC_DO_NOTHING)
        before = sc.sc_report_handler.get_count(sc.SC_ERROR)
        try:
            sc.sc_trace(trace, native.time_pointer(samples), "time")
            sc.sc_trace(trace, native.event_pointer(samples), "event")
            after = sc.sc_report_handler.get_count(sc.SC_ERROR)
        finally:
            sc.sc_report_handler.set_actions(report_id, old_actions)
        assert after == before + 2, (before, after)

    ignored_before = sc.sc_report_handler.get_count(sc.SC_WARNING)
    try:
        sc.sc_trace.__overload__("sc_core::sc_trace_file*,const void*,const std::string&")(
            trace, cppyy.nullptr, "ignored_pointer"
        )
    except Exception as error:
        raise AssertionError("native pointer trace overload raised") from error
    assert sc.sc_report_handler.get_count(sc.SC_WARNING) == ignored_before + 1
    return enumeration, signed_char, unsigned_char


fmt = sys.argv[1]
assert fmt in {"vcd", "wif"}, fmt
samples = cppyy.gbl.trace_format_matrix.Samples()
trace = (
    sc.sc_create_vcd_trace_file(f"python_trace_matrix_{fmt}")
    if fmt == "vcd"
    else sc.sc_create_wif_trace_file(f"python_trace_matrix_{fmt}")
)
trace.set_time_unit(1, sc.SC_PS if fmt == "vcd" else sc.SC_NS)
enumeration, signed_char, unsigned_char = register_trace(trace, samples, fmt)
sc.sc_start(1, sc.SC_NS)
assign_phase(samples, 1)
enumeration.value = 1
signed_char.value = bytes([243])
unsigned_char.value = 33
sc.sc_start(1, sc.SC_NS)
assign_phase(samples, 2)
enumeration.value = 2
signed_char.value = bytes([235])
unsigned_char.value = 65
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
assert sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
if fmt == "vcd":
    sc.sc_close_vcd_trace_file(trace)
else:
    sc.sc_close_wif_trace_file(trace)
print(f"TRACE_FORMAT_MATRIX_OK {fmt} {sc.sc_time_stamp()}")
