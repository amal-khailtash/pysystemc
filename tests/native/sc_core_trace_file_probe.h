/** @file sc_core_trace_file_probe.h
 * @brief Native derived recorder for the public sc_trace_file interface.
 * @details The recorder owns every sample passed to trace() and copies each
 * invocation into native strings. It retains no Python values or references.
 */
#ifndef PYSYSTEMC_SC_CORE_TRACE_FILE_PROBE_H
#define PYSYSTEMC_SC_CORE_TRACE_FILE_PROBE_H

#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <vector>

namespace sc_core
{

/** @brief Concrete test recorder implementing every abstract trace overload.
 * @details Sample values live in this object for its full lifetime. Calls are
 * synchronous and only copy values into records. The probe is native-derived;
 * it does not test Python implementations of virtual methods or GIL callbacks.
 */
class TraceFileProbe : public sc_trace_file
{
 public:
  using fixed_type = sc_dt::sc_fixed<12, 4>;
  using fixed_fast_type = sc_dt::sc_fixed_fast<12, 4>;

  TraceFileProbe()
  {
    signed_wide_sample_ = -77;
    unsigned_wide_sample_ = 211;
  }

  /** @brief Record destruction through the protected base destructor. */
  ~TraceFileProbe() override
  {
    ++destructions_;
  }

  /** @brief Return the cumulative number of destroyed probe objects. */
  static int destruction_count()
  {
    return destructions_;
  }

  /** @brief Expose copied recorder output without transferring ownership. */
  const std::vector<std::string>& records() const
  {
    return records_;
  }

  /** @brief Expose the protected cycle hook through a public test entry point.
   * @param is_delta True for a delta cycle and false for a timed cycle.
   */
  void invoke_cycle(bool is_delta)
  {
    cycle(is_delta);
  }

  /** @brief Return stable enum literals for direct trace overload dispatch.
   * @return Borrowed null-terminated array valid while this probe lives.
   */
  const char** enum_literals()
  {
    return enum_literals_;
  }

  /** @brief Return stable, object-owned samples for direct Python overload calls.
   * @details Every returned reference remains valid while this probe lives.
   */
#define PYSYSTEMC_TRACE_SAMPLE(TYPE, NAME) \
  TYPE& NAME()                             \
  {                                        \
    return NAME##_;                        \
  }
  PYSYSTEMC_TRACE_SAMPLE(sc_event, event_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_time, time_sample)
  PYSYSTEMC_TRACE_SAMPLE(bool, bool_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::sc_bit, bit_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::sc_logic, logic_sample)
  PYSYSTEMC_TRACE_SAMPLE(unsigned char, uchar_sample)
  PYSYSTEMC_TRACE_SAMPLE(unsigned short, ushort_sample)
  PYSYSTEMC_TRACE_SAMPLE(unsigned int, uint_sample)
  PYSYSTEMC_TRACE_SAMPLE(unsigned long, ulong_sample)
  PYSYSTEMC_TRACE_SAMPLE(char, char_sample)
  PYSYSTEMC_TRACE_SAMPLE(short, short_sample)
  PYSYSTEMC_TRACE_SAMPLE(int, int_sample)
  PYSYSTEMC_TRACE_SAMPLE(long, long_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::int64, int64_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::uint64, uint64_sample)
  PYSYSTEMC_TRACE_SAMPLE(float, float_sample)
  PYSYSTEMC_TRACE_SAMPLE(double, double_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::sc_int<13>, sc_int_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::sc_uint<17>, sc_uint_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::sc_signed, signed_wide_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::sc_unsigned, unsigned_wide_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::sc_fxval, fxval_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::sc_fxval_fast, fxval_fast_sample)
  PYSYSTEMC_TRACE_SAMPLE(fixed_type, fixed_sample)
  PYSYSTEMC_TRACE_SAMPLE(fixed_fast_type, fixed_fast_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::sc_bv<3>, bits_sample)
  PYSYSTEMC_TRACE_SAMPLE(sc_dt::sc_lv<3>, logic_vector_sample)
#undef PYSYSTEMC_TRACE_SAMPLE

  /** @brief Append one native dispatch record with stable value formatting. */
  template <typename Value>
  void record(const char* type, const std::string& name, const Value& value)
  {
    std::ostringstream stream;
    stream << "trace|" << type << '|' << name << '|' << value;
    records_.push_back(stream.str());
  }

  /** @brief Record an event by its stable native name. */
  void record(const char* type, const std::string& name, const sc_event& event)
  {
    records_.push_back(std::string("trace|") + type + '|' + name + '|' + event.name());
  }

  /** @brief Append one native dispatch record including the requested width. */
  template <typename Value>
  void record_width(const char* type, const std::string& name, const Value& value, int width)
  {
    std::ostringstream stream;
    stream << "trace|" << type << '|' << name << '|' << value << '|' << width;
    records_.push_back(stream.str());
  }

  /** @brief Preserve unsigned-byte values as numbers in recorder output. */
  void record_width(const char* type, const std::string& name, unsigned char value, int width)
  {
    std::ostringstream stream;
    stream << "trace|" << type << '|' << name << '|' << static_cast<unsigned int>(value) << '|' << width;
    records_.push_back(stream.str());
  }

#define PYSYSTEMC_TRACE_A(TYPE, LABEL)                             \
  void trace(const TYPE& object, const std::string& name) override \
  {                                                                \
    record(LABEL, name, object);                                   \
  }
#define PYSYSTEMC_TRACE_B(TYPE, LABEL)                                        \
  void trace(const TYPE& object, const std::string& name, int width) override \
  {                                                                           \
    record_width(LABEL, name, object, width);                                 \
  }

  PYSYSTEMC_TRACE_A(sc_event, "event")
  PYSYSTEMC_TRACE_A(sc_time, "time")
  PYSYSTEMC_TRACE_A(bool, "bool")
  PYSYSTEMC_TRACE_A(sc_dt::sc_bit, "sc_bit")
  PYSYSTEMC_TRACE_A(sc_dt::sc_logic, "sc_logic")
  PYSYSTEMC_TRACE_B(unsigned char, "unsigned char")
  PYSYSTEMC_TRACE_B(unsigned short, "unsigned short")
  PYSYSTEMC_TRACE_B(unsigned int, "unsigned int")
  PYSYSTEMC_TRACE_B(unsigned long, "unsigned long")
  PYSYSTEMC_TRACE_B(char, "char")
  PYSYSTEMC_TRACE_B(short, "short")
  PYSYSTEMC_TRACE_B(int, "int")
  PYSYSTEMC_TRACE_B(long, "long")
  PYSYSTEMC_TRACE_B(sc_dt::int64, "sc_dt::int64")
  PYSYSTEMC_TRACE_B(sc_dt::uint64, "sc_dt::uint64")
  PYSYSTEMC_TRACE_A(float, "float")
  PYSYSTEMC_TRACE_A(double, "double")
  PYSYSTEMC_TRACE_A(sc_dt::sc_int_base, "sc_int_base")
  PYSYSTEMC_TRACE_A(sc_dt::sc_uint_base, "sc_uint_base")
  PYSYSTEMC_TRACE_A(sc_dt::sc_signed, "sc_signed")
  PYSYSTEMC_TRACE_A(sc_dt::sc_unsigned, "sc_unsigned")
  PYSYSTEMC_TRACE_A(sc_dt::sc_fxval, "sc_fxval")
  PYSYSTEMC_TRACE_A(sc_dt::sc_fxval_fast, "sc_fxval_fast")
  PYSYSTEMC_TRACE_A(sc_dt::sc_fxnum, "sc_fxnum")
  PYSYSTEMC_TRACE_A(sc_dt::sc_fxnum_fast, "sc_fxnum_fast")
  PYSYSTEMC_TRACE_A(sc_dt::sc_bv_base, "sc_bv_base")
  PYSYSTEMC_TRACE_A(sc_dt::sc_lv_base, "sc_lv_base")

  void trace(const unsigned int& object, const std::string& name, const char** literals) override
  {
    std::ostringstream stream;
    stream << "trace|enum|" << name << '|' << object << '|';
    if (literals != nullptr)
    {
      for (int index = 0; literals[index] != nullptr; ++index)
      {
        if (index != 0)
        {
          stream << ',';
        }
        stream << literals[index];
      }
    }
    records_.push_back(stream.str());
  }

  void write_comment(const std::string& comment) override
  {
    records_.push_back("comment|" + comment);
  }

  void delta_cycles(bool enabled) override
  {
    records_.push_back(enabled ? "delta|1" : "delta|0");
  }

  void set_time_unit(double value, sc_time_unit unit) override
  {
    std::ostringstream stream;
    stream << "unit|" << value << '|' << static_cast<int>(unit);
    records_.push_back(stream.str());
  }

 protected:
  /** @brief Record kernel-cycle dispatch received through the protected hook. */
  void cycle(bool is_delta) override
  {
    records_.push_back(is_delta ? "cycle|delta" : "cycle|timed");
  }

 private:
  sc_event event_sample_{"event_sample"};
  sc_time time_sample_{7, SC_NS};
  bool bool_sample_{true};
  sc_dt::sc_bit bit_sample_{sc_dt::SC_LOGIC_1};
  sc_dt::sc_logic logic_sample_{sc_dt::SC_LOGIC_1};
  unsigned char uchar_sample_{65};
  unsigned short ushort_sample_{501};
  unsigned int uint_sample_{1001};
  unsigned long ulong_sample_{2001};
  char char_sample_{'c'};
  short short_sample_{-301};
  int int_sample_{-1001};
  long long_sample_{-2001};
  sc_dt::int64 int64_sample_{-3001};
  sc_dt::uint64 uint64_sample_{4001};
  float float_sample_{1.25F};
  double double_sample_{2.5};
  sc_dt::sc_int<13> sc_int_sample_{-77};
  sc_dt::sc_uint<17> sc_uint_sample_{211};
  sc_dt::sc_signed signed_wide_sample_{9};
  sc_dt::sc_unsigned unsigned_wide_sample_{9};
  sc_dt::sc_fxval fxval_sample_{1.25};
  sc_dt::sc_fxval_fast fxval_fast_sample_{2.5};
  fixed_type fixed_sample_{3.25};
  fixed_fast_type fixed_fast_sample_{2.5};
  sc_dt::sc_bv<3> bits_sample_{"101"};
  sc_dt::sc_lv<3> logic_vector_sample_{"10Z"};
  char enum_first_[5]{"IDLE"};
  char enum_second_[5]{"BUSY"};
  const char* enum_literals_[3]{enum_first_, enum_second_, nullptr};
  std::vector<std::string> records_;
  inline static int destructions_ = 0;
};

#undef PYSYSTEMC_TRACE_A
#undef PYSYSTEMC_TRACE_B

static_assert(std::is_abstract_v<sc_trace_file>, "sc_trace_file must remain abstract");
static_assert(!std::is_destructible_v<sc_trace_file>, "sc_trace_file destructor is protected");

}  // namespace sc_core

#endif
