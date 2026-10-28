/** @file sc_dt_integer_bitrefs_probe.h
 * @brief Native-only access to integer bit-reference pools and const views.
 * @details Bit-reference objects returned by SystemC owners borrow those
 * owners and occupy reusable pool slots. These helpers retain no Python
 * objects, callbacks, owners, streams or buffers. Calls are synchronous with
 * the Python GIL held and do not interact with the SystemC scheduler.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_INTEGER_BITREFS_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_INTEGER_BITREFS_PROBE_H

#include <sysc/datatypes/int/sc_int.h>
#include <sysc/datatypes/int/sc_signed.h>
#include <sysc/datatypes/int/sc_uint.h>
#include <sysc/datatypes/int/sc_unsigned.h>
#include <sysc/utils/sc_report.h>
#include <sysc/utils/sc_report_handler.h>

#include <cstdint>
#include <type_traits>

namespace sc_dt_integer_bitrefs_probe
{

using Int64 = sc_dt::int64;
using UInt64 = sc_dt::uint64;

/** @brief Expose the protected signed mutable bit-reference pool to tests.
 * @details The returned pool remains library-owned and must never be deleted
 * or reset by the caller. The helper exposes diagnostics only.
 */
struct signed_pool_access : sc_dt::sc_signed_bitref
{
  /** @brief Return the library-owned signed bit-reference pool. */
  static sc_core::sc_vpool<sc_dt::sc_signed_bitref>& get_pool()
  {
    return m_pool;
  }
};

/** @brief Expose the protected bounded signed bit-reference pool to tests. */
struct int_pool_access : sc_dt::sc_int_bitref
{
  /** @brief Return the library-owned bounded signed bit-reference pool. */
  static sc_core::sc_vpool<sc_dt::sc_int_bitref>& get_pool()
  {
    return m_pool;
  }
};

/** @brief Expose the protected bounded unsigned bit-reference pool to tests. */
struct uint_pool_access : sc_dt::sc_uint_bitref
{
  /** @brief Return the library-owned bounded unsigned bit-reference pool. */
  static sc_core::sc_vpool<sc_dt::sc_uint_bitref>& get_pool()
  {
    return m_pool;
  }
};

/** @brief Expose the protected unsigned mutable bit-reference pool to tests.
 * @details The returned pool remains library-owned and must never be deleted
 * or reset by the caller. The helper exposes diagnostics only.
 */
struct unsigned_pool_access : sc_dt::sc_unsigned_bitref
{
  /** @brief Return the library-owned unsigned bit-reference pool. */
  static sc_core::sc_vpool<sc_dt::sc_unsigned_bitref>& get_pool()
  {
    return m_pool;
  }
};

/** @brief Get a borrowed const integer view from a bounded signed owner.
 * @param owner Owner that remains alive for the returned view's use.
 * @param index Valid bit index in owner.
 * @return Reference to a reusable proxy shell that borrows the supplied owner.
 */
inline const sc_dt::sc_int_bitref_r& const_int_view(const sc_dt::sc_int_base& owner, int index)
{
  return owner[index];
}

/** @brief Get a borrowed const bit view from a bounded unsigned owner.
 * @param owner Owner that remains alive for the returned view's use.
 * @param index Valid bit index in owner.
 * @return Reference to a reusable proxy shell that borrows the supplied owner.
 */
inline const sc_dt::sc_uint_bitref_r& const_uint_view(const sc_dt::sc_uint_base& owner, int index)
{
  return owner[index];
}

/** @brief Get a borrowed const bit view from an arbitrary-width signed owner.
 * @param owner Owner that remains alive for the returned view's use.
 * @param index Valid bit index in owner.
 * @return Reference to a reusable proxy shell that borrows the supplied owner.
 */
inline const sc_dt::sc_signed_bitref_r& const_signed_view(const sc_dt::sc_signed& owner, int index)
{
  return owner[index];
}

/** @brief Get a borrowed const bit view from an arbitrary-width unsigned owner.
 * @param owner Owner that remains alive for the returned view's use.
 * @param index Valid bit index in owner.
 * @return Reference to a reusable proxy shell that borrows the supplied owner.
 */
inline const sc_dt::sc_unsigned_bitref_r& const_unsigned_view(const sc_dt::sc_unsigned& owner, int index)
{
  return owner[index];
}

/** @brief Select the const integer proxy for any supported native owner.
 * @param owner Live owner whose const selection returns a borrowed shell.
 * @param index Valid bit index in owner.
 * @return Reference to the owner's reusable const-view shell.
 */
inline const sc_dt::sc_int_bitref_r& const_view_for(const sc_dt::sc_int_base& owner, int index)
{
  return const_int_view(owner, index);
}

/** @brief Select the const unsigned proxy for any bounded unsigned owner. */
inline const sc_dt::sc_uint_bitref_r& const_view_for(const sc_dt::sc_uint_base& owner, int index)
{
  return const_uint_view(owner, index);
}

/** @brief Select the const proxy for any arbitrary-width signed owner. */
inline const sc_dt::sc_signed_bitref_r& const_view_for(const sc_dt::sc_signed& owner, int index)
{
  return const_signed_view(owner, index);
}

/** @brief Select the const proxy for any arbitrary-width unsigned owner. */
inline const sc_dt::sc_unsigned_bitref_r& const_view_for(const sc_dt::sc_unsigned& owner, int index)
{
  return const_unsigned_view(owner, index);
}

/** @brief Return a bounded owner's bit-reference pool capacity. */
inline std::size_t int_pool_size()
{
  return int_pool_access::get_pool().size();
}

/** @brief Return an unsigned bounded owner's bit-reference pool capacity. */
inline std::size_t uint_pool_size()
{
  return uint_pool_access::get_pool().size();
}

/** @brief Return a signed arbitrary-width owner's bit-reference pool capacity. */
inline std::size_t signed_pool_size()
{
  return signed_pool_access::get_pool().size();
}

/** @brief Return an unsigned arbitrary-width owner's bit-reference pool capacity. */
inline std::size_t unsigned_pool_size()
{
  return unsigned_pool_access::get_pool().size();
}

/** @brief Set SystemC errors to throw so invalid bit indexes are catchable.
 * @return Previous process-global error actions, to be restored after use.
 * @note This mutates process-global SystemC state and is only suitable for the
 * isolated one-shot subprocess used by the test.
 */
inline sc_core::sc_actions throw_errors()
{
  return sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
}

/** @brief Restore process-global SystemC error actions.
 * @param actions Value returned by throw_errors().
 */
inline void restore_errors(sc_core::sc_actions actions)
{
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, actions);
}

static_assert(std::is_copy_constructible<sc_dt::sc_int_bitref>::value, "sc_int bitref copy constructor");
static_assert(std::is_copy_constructible<sc_dt::sc_uint_bitref>::value, "sc_uint bitref copy constructor");
static_assert(std::is_copy_constructible<sc_dt::sc_signed_bitref>::value, "sc_signed bitref copy constructor");
static_assert(std::is_copy_constructible<sc_dt::sc_unsigned_bitref>::value, "sc_unsigned bitref copy constructor");
static_assert(!std::is_default_constructible<sc_dt::sc_signed_bitref>::value,
              "sc_signed bitref default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_unsigned_bitref>::value,
              "sc_unsigned bitref default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_int_bitref>::value,
              "sc_int bitref default constructor stays non-public");
static_assert(!std::is_default_constructible<sc_dt::sc_uint_bitref>::value,
              "sc_uint bitref default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_int_bitref_r>::value,
              "sc_int bitref r default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_uint_bitref_r>::value,
              "sc_uint bitref r default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_signed_bitref_r>::value,
              "sc_signed bitref r default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_unsigned_bitref_r>::value,
              "sc_unsigned bitref r default constructor stays protected");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_int_base&>()[0]), const sc_dt::sc_int_bitref_r&>::value,
  "sc_int const selection returns the r-value proxy");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_uint_base&>()[0]), const sc_dt::sc_uint_bitref_r&>::value,
  "sc_uint const selection returns the r-value proxy");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_signed&>()[0]), const sc_dt::sc_signed_bitref_r&>::value,
  "sc_signed const selection returns the r-value proxy");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_unsigned&>()[0]), const sc_dt::sc_unsigned_bitref_r&>::value,
  "sc_unsigned const selection returns the r-value proxy");

}  // namespace sc_dt_integer_bitrefs_probe

#endif  // PYSYSTEMC_TEST_SC_DT_INTEGER_BITREFS_PROBE_H
