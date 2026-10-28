/** @file sc_dt_values_probe.h
 * @brief Typed native consumers for small sc_dt enum, constant and scalar contracts.
 * @details All arguments are borrowed and consumed synchronously. The helpers
 * retain no Python objects, callbacks, streams or pointers. Python calls them
 * with the GIL held; they do not enter or mutate the SystemC scheduler.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_VALUES_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_VALUES_PROBE_H

#include <cstdint>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>

namespace sc_dt_values_probe
{

/** @brief Return an enum's underlying integer through its exact deduced type. */
template <typename Enum>
int enum_value(Enum value)
{
  static_assert(std::is_enum<Enum>::value, "value must be a native enum");
  return static_cast<int>(value);
}

/** @brief Format one fixed-point enum through its exact native overload. */
inline std::string format(sc_dt::sc_enc value)
{
  return sc_dt::to_string(value);
}

/** @brief Format one fixed-point enum through its exact native overload. */
inline std::string format(sc_dt::sc_fmt value)
{
  return sc_dt::to_string(value);
}

/** @brief Format one fixed-point enum through its exact native overload. */
inline std::string format(sc_dt::sc_numrep value)
{
  return sc_dt::to_string(value);
}

/** @brief Format one fixed-point enum through its exact native overload. */
inline std::string format(sc_dt::sc_o_mode value)
{
  return sc_dt::to_string(value);
}

/** @brief Format one fixed-point enum through its exact native overload. */
inline std::string format(sc_dt::sc_q_mode value)
{
  return sc_dt::to_string(value);
}

/** @brief Format one fixed-point enum through its exact native overload. */
inline std::string format(sc_dt::sc_switch value)
{
  return sc_dt::to_string(value);
}

/** @brief Expose each enum formatter with a distinct binding name. */
inline std::string format_enc(sc_dt::sc_enc value)
{
  return format(value);
}
/** @brief Expose each enum formatter with a distinct binding name. */
inline std::string format_fmt(sc_dt::sc_fmt value)
{
  return format(value);
}
/** @brief Expose each enum formatter with a distinct binding name. */
inline std::string format_numrep(sc_dt::sc_numrep value)
{
  return format(value);
}
/** @brief Expose each enum formatter with a distinct binding name. */
inline std::string format_o_mode(sc_dt::sc_o_mode value)
{
  return format(value);
}
/** @brief Expose each enum formatter with a distinct binding name. */
inline std::string format_q_mode(sc_dt::sc_q_mode value)
{
  return format(value);
}
/** @brief Expose each enum formatter with a distinct binding name. */
inline std::string format_switch(sc_dt::sc_switch value)
{
  return format(value);
}

/** @brief Stream a logic value using the native sc_logic insertion operator. */
inline std::string format_logic(sc_dt::sc_logic_value_t value)
{
  std::ostringstream stream;
  stream << sc_dt::sc_logic(value);
  return stream.str();
}

/** @brief Return a logic constant's enum value without copying its identity. */
inline int logic_value(const sc_dt::sc_logic& value)
{
  return static_cast<int>(value.value());
}

/** @brief Return the address of a named native logic constant.
 * @param group Select uppercase constants with 0 or lowercase aliases with 1.
 * @param index Select zero, one, unknown or high-impedance in declaration order.
 * @return Address of the corresponding library-owned const object.
 */
inline std::uintptr_t logic_address(int group, int index)
{
  if (group == 0)
  {
    switch (index)
    {
    case 0:
      return reinterpret_cast<std::uintptr_t>(&sc_dt::SC_LOGIC_0);
    case 1:
      return reinterpret_cast<std::uintptr_t>(&sc_dt::SC_LOGIC_1);
    case 2:
      return reinterpret_cast<std::uintptr_t>(&sc_dt::SC_LOGIC_X);
    case 3:
      return reinterpret_cast<std::uintptr_t>(&sc_dt::SC_LOGIC_Z);
    }
  }
  if (group == 1)
  {
    switch (index)
    {
    case 0:
      return reinterpret_cast<std::uintptr_t>(&sc_dt::sc_logic_0);
    case 1:
      return reinterpret_cast<std::uintptr_t>(&sc_dt::sc_logic_1);
    case 2:
      return reinterpret_cast<std::uintptr_t>(&sc_dt::sc_logic_X);
    case 3:
      return reinterpret_cast<std::uintptr_t>(&sc_dt::sc_logic_Z);
    }
  }
  return 0;
}

/** @brief Set native stream base and show-base flags for manipulator checks. */
inline void configure_stream(std::ostream& stream, int base, bool show_base)
{
  const std::ios_base::fmtflags base_flag = base == 16  ? std::ios_base::hex
                                            : base == 8 ? std::ios_base::oct
                                                        : std::ios_base::dec;
  stream.setf(base_flag, std::ios_base::basefield);
  if (show_base)
    stream.setf(std::ios_base::showbase);
  else
    stream.unsetf(std::ios_base::showbase);
}

/** @brief Preserve the native signed 64-bit alias at a call boundary. */
inline sc_dt::int64 echo_int64(sc_dt::int64 value)
{
  return value;
}

/** @brief Preserve the native unsigned 64-bit alias at a call boundary. */
inline sc_dt::uint64 echo_uint64(sc_dt::uint64 value)
{
  return value;
}

/** @brief Preserve the native arithmetic digit alias at a call boundary. */
inline sc_dt::sc_digit echo_digit(sc_dt::sc_digit value)
{
  return value;
}

/** @brief Apply the native unsigned digit-width conversion to a 64-bit value. */
inline sc_dt::sc_digit narrow_digit(sc_dt::uint64 value)
{
  return static_cast<sc_dt::sc_digit>(value);
}

static_assert(std::is_same<sc_dt::int64, long long>::value, "int64 alias");
static_assert(std::is_same<sc_dt::uint64, unsigned long long>::value, "uint64 alias");
static_assert(std::is_same<sc_dt::sc_digit, unsigned int>::value, "digit alias");
static_assert(std::is_same<decltype(sc_dt::SC_LOGIC_0), const sc_dt::sc_logic>::value, "logic constant type");

}  // namespace sc_dt_values_probe

#endif  // PYSYSTEMC_TEST_SC_DT_VALUES_PROBE_H
