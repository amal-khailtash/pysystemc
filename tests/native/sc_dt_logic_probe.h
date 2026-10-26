/** @file sc_dt_logic_probe.h
 * @brief Small stream adapters for public sc_bit and sc_logic operations.
 *
 * The adapters own no Python references and never call Python. Values are
 * passed or returned by value; streams and scalar arguments stay local to the
 * call. Native SystemC reports may propagate through cppyy as C++ exceptions.
 */
#ifndef TESTS_NATIVE_SC_DT_LOGIC_PROBE_H
#define TESTS_NATIVE_SC_DT_LOGIC_PROBE_H

#include <sysc/datatypes/bit/sc_bit.h>
#include <sysc/datatypes/bit/sc_logic.h>
#include <sysc/utils/sc_report.h>
#include <sysc/utils/sc_report_handler.h>

#include <sstream>
#include <string>

namespace sc_dt_logic_probe
{

using Unsigned = unsigned int;
using Long = long;
using UnsignedLong = unsigned long;
using Int64 = sc_dt::int64;
using UInt64 = sc_dt::uint64;

/** @brief Non-owning counters for derived-value destruction observations.
 * @details Counter storage must outlive every CountedBit and CountedLogic that
 *          points to it. It is created and destroyed by the caller.
 */
struct DestructionCounters
{
  unsigned bit_count = 0;
  unsigned logic_count = 0;
};

/** @brief Count destruction of a Python-owned sc_bit-derived object.
 * @details The counter pointer is borrowed and must outlive this object. The
 *          wrapper owns the derived C++ instance; its destructor does not call
 *          Python and has no GIL requirement.
 */
class CountedBit : public sc_dt::sc_bit
{
 public:
  /** @brief Construct a tracked bit value.
   * @param counters Borrowed counter storage that must outlive this object.
   * @param value Initial native bit value.
   */
  CountedBit(DestructionCounters& counters, bool value) : sc_dt::sc_bit(value), counters_(&counters)
  {
  }

  /** @brief Increment the borrowed counter when the native object is deleted. */
  ~CountedBit()
  {
    ++counters_->bit_count;
  }

 private:
  DestructionCounters* counters_;
};

/** @brief Count destruction of a Python-owned sc_logic-derived object.
 * @details The counter pointer is borrowed and must outlive this object. The
 *          wrapper owns the derived C++ instance; its destructor does not call
 *          Python and has no GIL requirement.
 */
class CountedLogic : public sc_dt::sc_logic
{
 public:
  /** @brief Construct a tracked four-state value.
   * @param counters Borrowed counter storage that must outlive this object.
   * @param value Initial four-state character.
   */
  CountedLogic(DestructionCounters& counters, char value) : sc_dt::sc_logic(value), counters_(&counters)
  {
  }

  /** @brief Increment the borrowed counter when the native object is deleted. */
  ~CountedLogic()
  {
    ++counters_->logic_count;
  }

 private:
  DestructionCounters* counters_;
};

/** @brief Temporarily suppress SystemC error reports for invalid-value fallback checks.
 * @return Previous global error actions; restore them after the bounded check.
 * @note This changes process-global report state and is only safe in the isolated
 *       one-shot test process. No Python callback or GIL interaction occurs.
 */
inline sc_core::sc_actions suppress_errors()
{
  return sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_DO_NOTHING);
}

/** @brief Change SystemC error reports to throw so fatal legacy validation can be caught.
 * @return Previous global error actions; restore them after the bounded check.
 * @note This changes process-global report state and is only safe in the isolated
 *       one-shot test process. No Python callback or GIL interaction occurs.
 */
inline sc_core::sc_actions throw_errors()
{
  return sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
}

/** @brief Restore the previous process-global SystemC error action.
 * @param actions Value returned by suppress_errors().
 * @note No Python callback or GIL interaction occurs.
 */
inline void restore_errors(sc_core::sc_actions actions)
{
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, actions);
}

/** @brief Print a borrowed bit into a locally owned stream and copy its text.
 * @param value Native bit value; borrowed for this call only.
 * @return The printed character.
 * @throws std::ios_base::failure If the local output stream fails.
 * @note No callback or GIL interaction occurs.
 */
inline std::string print_bit(const sc_dt::sc_bit& value)
{
  std::ostringstream output;
  value.print(output);
  return output.str();
}

/** @brief Print a borrowed four-state value into a local stream.
 * @param value Native logic value; borrowed for this call only.
 * @return The printed state character.
 * @throws std::ios_base::failure If the local output stream fails.
 * @note No callback or GIL interaction occurs.
 */
inline std::string print_logic(const sc_dt::sc_logic& value)
{
  std::ostringstream output;
  value.print(output);
  return output.str();
}

/** @brief Scan one bit from copied input text using the public member method.
 * @param text Null-terminated input copied into a local stream.
 * @return The scanned bit by value.
 * @throws std::ios_base::failure If stream extraction fails.
 * @note The returned object owns its value; no callback or GIL interaction occurs.
 */
inline sc_dt::sc_bit scan_bit(const char* text)
{
  std::istringstream input(text);
  sc_dt::sc_bit result;
  result.scan(input);
  return result;
}

/** @brief Scan one four-state value from copied input text.
 * @param text Null-terminated input copied into a local stream.
 * @return The scanned value by value.
 * @throws std::ios_base::failure If stream extraction fails.
 * @note The returned object owns its value; no callback or GIL interaction occurs.
 */
inline sc_dt::sc_logic scan_logic(const char* text)
{
  std::istringstream input(text);
  sc_dt::sc_logic result;
  result.scan(input);
  return result;
}

}  // namespace sc_dt_logic_probe

#endif  // TESTS_NATIVE_SC_DT_LOGIC_PROBE_H
