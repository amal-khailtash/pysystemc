/** @file sc_dt_concat_functions_probe.h
 * @brief Small native helpers for public sc_dt concatenation probes.
 * @details Vector concatenation results borrow their vector/proxy parents;
 *          every owner and nested proxy must outlive its view. Integer
 *          concatenation shells borrow their native value operands and use
 *          SystemC's global shell pool. Helpers retain no Python objects or
 *          callbacks, release no GIL state, and never enter the kernel.
 *          Native reports and allocation exceptions propagate to the caller.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_CONCAT_FUNCTIONS_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_CONCAT_FUNCTIONS_PROBE_H

#include <cxxabi.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>
#include <systemc>
#include <typeinfo>

namespace sc_dt_concat_functions_probe
{

/** @brief Construct a read-only borrowed range from a live vector owner.
 * @param owner Parent vector; it must outlive the returned range.
 * @param high Inclusive high endpoint.
 * @param low Inclusive low endpoint.
 * @return Read-only proxy borrowing owner storage.
 * @throws sc_core::sc_report If SystemC rejects the requested range.
 */
inline auto readonly_bv_range(const sc_dt::sc_bv_base& owner, int high, int low) -> decltype(owner.range(high, low))
{
  return owner.range(high, low);
}

/** @brief Construct a read-only bit reference from a live vector owner.
 * @param owner Parent vector; it must outlive the returned bit reference.
 * @param index Zero-based bit index.
 * @return Read-only proxy borrowing owner storage.
 * @throws sc_core::sc_report If SystemC rejects the requested index.
 */
inline auto readonly_bv_bit(const sc_dt::sc_bv_base& owner, int index) -> decltype(owner.bit(index))
{
  return owner.bit(index);
}

/** @brief Construct a read-only borrowed range from a live logic owner.
 * @param owner Parent vector; it must outlive the returned range.
 * @param high Inclusive high endpoint.
 * @param low Inclusive low endpoint.
 * @return Read-only proxy borrowing owner storage.
 * @throws sc_core::sc_report If SystemC rejects the requested range.
 */
inline auto readonly_lv_range(const sc_dt::sc_lv_base& owner, int high, int low) -> decltype(owner.range(high, low))
{
  return owner.range(high, low);
}

/** @brief Construct a read-only bit reference from a live logic-vector owner.
 * @param owner Parent vector; it must outlive the returned bit reference.
 * @param index Zero-based bit index.
 * @return Read-only proxy borrowing owner storage.
 * @throws sc_core::sc_report If SystemC rejects the requested index.
 */
inline auto readonly_lv_bit(const sc_dt::sc_lv_base& owner, int index) -> decltype(owner.bit(index))
{
  return owner.bit(index);
}

/** @brief Copy the printable spelling while the borrowed source is alive.
 * @tparam View A native vector, proxy or integer concatenation shell.
 * @param view Live native source; all its borrowed parents must still exist.
 * @return Owning standard string containing the native stream spelling.
 */
template <typename View>
inline std::string render(const View& view)
{
  std::ostringstream output;
  output << view;
  return output.str();
}

/** @brief Return the compiler's canonical ABI type spelling for a result.
 * @tparam View Exact native return type selected by the caller.
 * @param view Native result; borrowed parents must remain alive.
 * @return ABI type name used to compare Python and C++ overload results.
 */
template <typename View>
inline std::string result_type_name(const View& view)
{
  int status = 0;
  char* const demangled = abi::__cxa_demangle(typeid(view).name(), nullptr, nullptr, &status);
  std::string result = status == 0 ? demangled : typeid(view).name();
  std::free(demangled);
  result.erase(
    std::remove_if(result.begin(), result.end(), [](unsigned char character) { return std::isspace(character); }),
    result.end());
  return result;
}

/** @brief Return the exact public mutable bit-vector concatenation type.
 * @tparam Left Left parent or mutable proxy type.
 * @tparam Right Right parent or mutable proxy type.
 * @param left Live left operand; it remains borrowed by the result.
 * @param right Live right operand; it remains borrowed by the result.
 * @return Writable native concatenation proxy, borrowing both operands.
 */
template <typename Left, typename Right>
inline auto concat_mutable(Left& left, Right& right) -> decltype(sc_dt::concat(left, right))
{
  return sc_dt::concat(left, right);
}

/** @brief Return the exact public read-only concatenation type.
 * @tparam Left Read-only left operand type.
 * @tparam Right Read-only right operand type.
 * @param left Live left operand; it remains borrowed by the result.
 * @param right Live right operand; it remains borrowed by the result.
 * @return Read-only native concatenation proxy, borrowing both operands.
 */
template <typename Left, typename Right>
inline auto concat_readonly(const Left& left, const Right& right) -> decltype(sc_dt::concat(left, right))
{
  return sc_dt::concat(left, right);
}

}  // namespace sc_dt_concat_functions_probe

#endif  // PYSYSTEMC_TEST_SC_DT_CONCAT_FUNCTIONS_PROBE_H
