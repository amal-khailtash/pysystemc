/** @file sc_core_public_streams_probe.h
 * @brief Native helpers for public SystemC stream insertion comparisons.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_PUBLIC_STREAMS_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_PUBLIC_STREAMS_PROBE_H

#include <cstdint>
#include <ios>
#include <sstream>
#include <string>
#include <systemc>

namespace sc_core_public_streams_probe
{

/** @brief A caller-defined streamable payload used by native signal buffers and ports. */
struct Payload
{
  int value = 0; /**< Payload value copied into channel storage. */

  /** @brief Construct a payload value.
   * @param sample Integer value to retain by value.
   */
  explicit Payload(int sample = 0) : value(sample)
  {
  }

  /** @brief Compare payload values for SystemC channel update detection. */
  bool operator==(const Payload& other) const
  {
    return value == other.value;
  }
};

/** @brief Satisfy SystemC's dependent input-port trace lookup for Payload.
 * @param file Borrowed trace file; this stream-focused test never configures one.
 * @param payload Borrowed payload value retained by its channel.
 * @param name Borrowed trace name copied by no operation in this no-op adapter.
 * @details Ports instantiate their trace call even when tracing is disabled.
 * This probe-only overload adds no trace registration, callback, ownership
 * transfer, GIL operation or exception path.
 */
inline void sc_trace(sc_core::sc_trace_file*, const Payload&, const std::string&)
{
}

/** @brief Write a borrowed payload without transferring ownership.
 * @param stream Borrowed output stream retained by the caller.
 * @param payload Borrowed payload retained by its channel or caller.
 * @return The original stream for native insertion chaining.
 * @details The synchronous operator calls no Python code, releases no GIL, and
 * propagates native stream exceptions unchanged.
 */
inline std::ostream& operator<<(std::ostream& stream, const Payload& payload)
{
  return stream << "payload(" << payload.value << ')';
}

/** @brief A derived stream type loaded after Python stream adapters are installed. */
struct LateOstream : std::ostringstream
{
};

/** @brief Ports spanning native bool, wide integer, and custom payload values.
 * @details Ports borrow channels bound by the caller. No process callbacks or
 * Python references are retained; stream insertion remains synchronous with the
 * GIL held by the Python caller.
 */
class Ports final : public sc_core::sc_module
{
 public:
  sc_core::sc_in<bool> bool_input{"bool_input"};
  sc_core::sc_inout<bool> bool_inout{"bool_inout"};
  sc_core::sc_out<bool> bool_output{"bool_output"};
  sc_core::sc_in<sc_dt::sc_biguint<137>> wide_input{"wide_input"};
  sc_core::sc_inout<sc_dt::sc_biguint<137>> wide_inout{"wide_inout"};
  sc_core::sc_in<Payload> payload_input{"payload_input"};
  sc_core::sc_inout<Payload> payload_inout{"payload_inout"};

  /** @brief Construct unregistered ports; caller binds and retains each channel.
   * @param name Native module name retained by the SystemC hierarchy.
   */
  explicit Ports(sc_core::sc_module_name name) : sc_module(name)
  {
  }
};

/** @brief Return the canonical std::ostream address for a derived stream.
 * @param stream Borrowed stream retained by the caller.
 * @return The address of its native ostream subobject.
 * @details No callback, ownership transfer or GIL operation occurs.
 */
inline std::uintptr_t address(std::ostream& stream)
{
  return reinterpret_cast<std::uintptr_t>(&stream);
}

/** @brief Put a stream into badbit state to exercise native I/O failure mapping.
 * @param stream Borrowed stream retained by the caller.
 * @details The caller configures badbit exceptions. Native ios failure propagates;
 * no Python callbacks or GIL release occur.
 */
inline void fail_stream(std::ostream& stream)
{
  stream.exceptions(std::ios::badbit);
  stream.setstate(std::ios::badbit);
}

/** @brief Flatten newlines in one copied output string for native/Python comparison.
 * @param value Owned string value consumed by this helper.
 * @return A copied string with each newline replaced by a vertical bar.
 */
inline std::string flatten(std::string value)
{
  for (char& character : value)
    if (character == '\n') character = '|';
  return value;
}

}  // namespace sc_core_public_streams_probe

#endif  // PYSYSTEMC_TEST_SC_CORE_PUBLIC_STREAMS_PROBE_H
