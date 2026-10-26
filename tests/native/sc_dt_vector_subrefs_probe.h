/** @file sc_dt_vector_subrefs_probe.h
 * @brief Small helpers for auditing the public bit-vector proxy families.
 * @details All proxy results borrow the vector or proxy arguments supplied by
 *          the caller. Owners and nested proxy objects must remain alive until
 *          each view is no longer used. No helper retains Python references,
 *          invokes callbacks, enters the scheduler, or releases the GIL.
 *          Native SystemC reports may propagate as C++ exceptions.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_VECTOR_SUBREFS_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_VECTOR_SUBREFS_PROBE_H

#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>

namespace sc_dt_vector_subrefs_probe
{

using bv_subref = sc_dt::sc_subref<sc_dt::sc_bv_base>;
using bv_subref_r = sc_dt::sc_subref_r<sc_dt::sc_bv_base>;
using lv_subref = sc_dt::sc_subref<sc_dt::sc_lv_base>;
using lv_subref_r = sc_dt::sc_subref_r<sc_dt::sc_lv_base>;
using bv_concat_r = sc_dt::sc_concref_r<bv_subref_r, lv_subref_r>;
using bv_concat = sc_dt::sc_concref<bv_subref, lv_subref>;

/** @brief Return a stable 32-element bool source for proxy assignment probes.
 * @return Borrowed static storage; it lives until process exit.
 */
inline const bool* assignment_bool_bits()
{
  static const bool value[32] = {true,  false, true,  true, false, true,  false, false, true,  true, false,
                                 false, true,  false, true, false, true,  true,  false, false, true, false,
                                 true,  false, false, true, true,  false, true,  false, true,  false};
  return value;
}

/** @brief Return a stable two-state sc_logic source for assignment probes.
 * @return Borrowed static storage; it lives until process exit.
 */
inline const sc_dt::sc_logic* assignment_logic_bits()
{
  static const sc_dt::sc_logic value[32] = {
    sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_1,
    sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_0,
    sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_1,
    sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_0,
    sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_0, sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_0,
    sc_dt::SC_LOGIC_1, sc_dt::SC_LOGIC_0};
  return value;
}

/** @brief Return a stable arbitrary-precision unsigned assignment source.
 * @return Borrowed 32-bit unsigned integer with a mixed bit pattern.
 */
inline const sc_dt::sc_unsigned& assignment_unsigned_value()
{
  static const sc_dt::sc_unsigned value = []
  {
    sc_dt::sc_unsigned result(32);
    result = "10100101101001011100110000111100";
    return result;
  }();
  return value;
}

/** @brief Return a stable arbitrary-precision signed assignment source.
 * @return Borrowed 32-bit signed integer with a mixed bit pattern.
 */
inline const sc_dt::sc_signed& assignment_signed_value()
{
  static const sc_dt::sc_signed value = []
  {
    sc_dt::sc_signed result(32);
    result = "10100101101001011100110000111100";
    return result;
  }();
  return value;
}

/** @brief Return a stable fixed-width unsigned assignment source.
 * @return Borrowed 32-bit unsigned integer with a mixed bit pattern.
 */
inline const sc_dt::sc_uint_base& assignment_uint_value()
{
  static const sc_dt::sc_uint<32> value("10100101101001011100110000111100");
  return value;
}

/** @brief Return a stable fixed-width signed assignment source.
 * @return Borrowed 32-bit signed integer with a mixed bit pattern.
 */
inline const sc_dt::sc_int_base& assignment_int_value()
{
  static const sc_dt::sc_int<32> value(static_cast<std::int64_t>(0xA5A5CC3C));
  return value;
}

/** @brief Return the native vector proxy used for template assignment probes.
 * @return Borrowed 32-bit BV owner usable as sc_proxy<sc_bv_base>.
 */
inline const sc_dt::sc_bv<32>& assignment_proxy_value()
{
  static const sc_dt::sc_bv<32> value("10100101101001011100110000111100");
  return value;
}

static_assert(std::is_constructible<bv_subref, sc_dt::sc_bv_base&, int, int>::value,
              "mutable BV ranges have a public borrowing constructor");
static_assert(std::is_constructible<bv_subref_r, const sc_dt::sc_bv_base&, int, int>::value,
              "read-only BV ranges have a public borrowing constructor");
static_assert(std::is_constructible<lv_subref, sc_dt::sc_lv_base&, int, int>::value,
              "mutable LV ranges have a public borrowing constructor");
static_assert(std::is_constructible<lv_subref_r, const sc_dt::sc_lv_base&, int, int>::value,
              "read-only LV ranges have a public borrowing constructor");
static_assert(std::is_constructible<bv_subref, const bv_subref&>::value,
              "mutable BV range proxies have a public copy constructor");
static_assert(std::is_constructible<bv_subref_r, const bv_subref_r&>::value,
              "read-only BV range proxies have a public copy constructor");
static_assert(std::is_constructible<lv_subref, const lv_subref&>::value,
              "mutable LV range proxies have a public copy constructor");
static_assert(std::is_constructible<lv_subref_r, const lv_subref_r&>::value,
              "read-only LV range proxies have a public copy constructor");
static_assert(!std::is_default_constructible<bv_subref>::value && !std::is_default_constructible<bv_subref_r>::value &&
                !std::is_default_constructible<lv_subref>::value && !std::is_default_constructible<lv_subref_r>::value,
              "range proxies cannot be constructed without a live owner");
static_assert(!std::is_copy_assignable<bv_subref_r>::value && !std::is_copy_assignable<lv_subref_r>::value,
              "read-only range proxy assignment is disabled");
static_assert(std::is_copy_assignable<bv_subref>::value && std::is_copy_assignable<lv_subref>::value,
              "mutable range proxy assignment is public");
static_assert(std::is_constructible<bv_concat_r, const bv_subref_r&, const lv_subref_r&>::value,
              "read-only concatenations have a public borrowing constructor");
static_assert(std::is_constructible<bv_concat_r, const bv_concat_r&>::value &&
                !std::is_copy_assignable<bv_concat_r>::value,
              "read-only concatenations can be copied but not assigned");
static_assert(std::is_constructible<bv_concat, bv_subref&, lv_subref&>::value,
              "mutable concatenations have a public borrowing constructor");
static_assert(std::is_constructible<bv_concat, const bv_concat&>::value && std::is_copy_assignable<bv_concat>::value,
              "mutable concatenations can be copied and assigned");

/** @brief Return the exact native range type selected for a const BV owner.
 * @param owner Live bit-vector owner; it must outlive the returned proxy.
 * @param high Inclusive first endpoint, in the owner’s native bit numbering.
 * @param low Inclusive second endpoint, in the owner’s native bit numbering.
 * @return Read-only borrowed range proxy.
 * @throws sc_core::sc_report If either endpoint is outside the owner.
 */
inline bv_subref_r readonly_bv_range(const sc_dt::sc_bv_base& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Return the exact native range type selected for a const LV owner.
 * @param owner Live logic-vector owner; it must outlive the returned proxy.
 * @param high Inclusive first endpoint, in the owner’s native bit numbering.
 * @param low Inclusive second endpoint, in the owner’s native bit numbering.
 * @return Read-only borrowed range proxy.
 * @throws sc_core::sc_report If either endpoint is outside the owner.
 */
inline lv_subref_r readonly_lv_range(const sc_dt::sc_lv_base& owner, int high, int low)
{
  return owner.range(high, low);
}

/** @brief Report protected range metadata through a short-lived derived copy.
 * @tparam Proxy A read-only or writable sc_subref specialization.
 * @param view Borrowed range proxy; its parent must remain alive.
 * @return High endpoint, low endpoint, length, and reversed flag.
 * @details The derived copy only exposes protected state to this helper. Its
 *          base constructor preserves the original borrowed parent reference.
 */
template <typename Proxy>
inline std::string range_state(const Proxy& view)
{
  struct state_view : Proxy
  {
    explicit state_view(const Proxy& source) : Proxy(source)
    {
    }
    std::string describe() const
    {
      std::ostringstream output;
      output << this->m_hi << '\t' << this->m_lo << '\t' << this->m_len << '\t' << int(this->reversed());
      return output.str();
    }
  } inspected(view);
  return inspected.describe();
}

/** @brief Read protected child lengths and ownership flags from a concat copy.
 * @tparam Proxy A mutable or read-only sc_concref specialization.
 * @param view Borrowed concatenation whose child proxies and their owners live.
 * @return Left and right lengths, delete flag, and shared-reference count.
 * @details Copying the view briefly increments its native reference count; the
 *          returned count therefore reflects the helper's live inspection copy.
 */
template <typename Proxy>
inline std::string concat_state(const Proxy& view)
{
  struct state_view : Proxy
  {
    explicit state_view(const Proxy& source) : Proxy(source)
    {
    }
    std::string describe() const
    {
      std::ostringstream output;
      output << this->m_left.length() << '\t' << this->m_right.length() << '\t' << this->m_delete << '\t'
             << this->m_refs;
      return output.str();
    }
  } inspected(view);
  return inspected.describe();
}

/** @brief Copy the formatted proxy text while the source owner remains live.
 * @tparam Proxy An sc_proxy-derived view.
 * @param view Borrowed source proxy.
 * @return Native SystemC bit-string spelling.
 */
template <typename Proxy>
inline std::string render(const Proxy& view)
{
  std::ostringstream output;
  output << view;
  return output.str();
}

/** @brief Print a proxy using its public stream member.
 * @tparam Proxy An sc_proxy-derived view.
 * @param view Borrowed source proxy.
 * @return Copied text written by the native print method.
 */
template <typename Proxy>
inline std::string print(const Proxy& view)
{
  std::ostringstream output;
  view.print(output);
  return output.str();
}

/** @brief Return a small type-trait signature for a concrete range proxy.
 * @tparam Proxy Range proxy specialization under test.
 * @return Whether its value type is bool, value type is four-state, and it is
 *         copy assignable.
 */
template <typename Proxy>
inline std::string traits()
{
  std::ostringstream output;
  output << int(std::is_same<typename Proxy::value_type, bool>::value) << '\t'
         << int(std::is_same<typename Proxy::value_type, sc_dt::sc_logic_value_t>::value) << '\t'
         << int(std::is_copy_assignable<Proxy>::value);
  return output.str();
}

/** @brief Return the BV writable range trait signature without template lookup.
 * @return Bool-value, four-state-value, and copy-assignable flags.
 */
inline std::string bv_mutable_traits()
{
  return traits<bv_subref>();
}

/** @brief Return the BV read-only range trait signature without template lookup.
 * @return Bool-value, four-state-value, and copy-assignable flags.
 */
inline std::string bv_readonly_traits()
{
  return traits<bv_subref_r>();
}

/** @brief Return the LV writable range trait signature without template lookup.
 * @return Bool-value, four-state-value, and copy-assignable flags.
 */
inline std::string lv_mutable_traits()
{
  return traits<lv_subref>();
}

/** @brief Return the LV read-only range trait signature without template lookup.
 * @return Bool-value, four-state-value, and copy-assignable flags.
 */
inline std::string lv_readonly_traits()
{
  return traits<lv_subref_r>();
}

/** @brief Return writable BV/LV concatenation trait flags.
 * @return Bool-value, four-state-value, and copy-assignable flags.
 */
inline std::string concat_mutable_traits()
{
  return traits<bv_concat>();
}

/** @brief Return read-only BV/LV concatenation trait flags.
 * @return Bool-value, four-state-value, and copy-assignable flags.
 */
inline std::string concat_readonly_traits()
{
  return traits<bv_concat_r>();
}

/** @brief Report whether all proxy construction and assignment traits hold.
 * @return True for public borrowing/copy constructors, disabled defaults,
 *         read-only assignment, and writable proxy assignment.
 */
inline bool construction_traits()
{
  return std::is_constructible<bv_subref, sc_dt::sc_bv_base&, int, int>::value &&
         std::is_constructible<bv_subref_r, const sc_dt::sc_bv_base&, int, int>::value &&
         std::is_constructible<lv_subref, sc_dt::sc_lv_base&, int, int>::value &&
         std::is_constructible<lv_subref_r, const sc_dt::sc_lv_base&, int, int>::value &&
         std::is_constructible<bv_concat, bv_subref&, lv_subref&>::value &&
         std::is_constructible<bv_concat_r, const bv_subref_r&, const lv_subref_r&>::value &&
         !std::is_default_constructible<bv_subref>::value && !std::is_default_constructible<bv_subref_r>::value &&
         !std::is_copy_assignable<bv_subref_r>::value && !std::is_copy_assignable<bv_concat_r>::value &&
         std::is_copy_assignable<bv_subref>::value && std::is_copy_assignable<bv_concat>::value;
}

/** @brief Exercise native range scanning with a supplied string.
 * @tparam Proxy Writable range proxy.
 * @param view Borrowed mutable destination proxy.
 * @param text Input bits consumed by the public scan member.
 * @return Resulting native bit-string spelling.
 */
template <typename Proxy>
inline std::string scan(Proxy& view, const std::string& text)
{
  std::istringstream input(text);
  view.scan(input);
  return render(view);
}

/** @brief Consume nested native read-only concatenations while all owners live.
 * @param first Live BV owner.
 * @param second Live LV owner.
 * @param third Live LV owner.
 * @return Native text from a nested sc_concref_r expression.
 */
inline std::string nested_read_concat(const sc_dt::sc_bv_base& first, const sc_dt::sc_lv_base& second,
                                      const sc_dt::sc_lv_base& third)
{
  const auto first_range = first.range(31, 16);
  const auto second_range = second.range(39, 24);
  const auto third_range = third.range(47, 32);
  const auto inner = sc_dt::concat(first_range, second_range);
  const auto outer = sc_dt::concat(inner, third_range);
  return render(outer);
}

/** @brief Catch a native invalid-range report and return its stable category.
 * @param owner Live BV owner.
 * @return True when the requested range is rejected by SystemC.
 */
inline bool rejects_invalid_bv_range(const sc_dt::sc_bv_base& owner)
{
  try
  {
    (void)owner.range(owner.length(), 0);
  }
  catch (const sc_core::sc_report&)
  {
    return true;
  }
  return false;
}

}  // namespace sc_dt_vector_subrefs_probe

#endif  // PYSYSTEMC_TEST_SC_DT_VECTOR_SUBREFS_PROBE_H
