/** @file sc_dt_integer_subrefs_probe.h
 * @brief Native-only diagnostics for integer range-reference families.
 * @details Range proxies borrow their parent integer and native pool slot.
 * Helpers return library-owned proxy references and never reset their pools.
 * Calls are synchronous with the Python GIL held; no callbacks or kernel
 * scheduling are involved.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_INTEGER_SUBREFS_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_INTEGER_SUBREFS_PROBE_H

#include <sysc/datatypes/int/sc_int.h>
#include <sysc/datatypes/int/sc_signed.h>
#include <sysc/datatypes/int/sc_uint.h>
#include <sysc/datatypes/int/sc_unsigned.h>
#include <sysc/datatypes/misc/sc_concatref.h>
#include <sysc/utils/sc_report.h>
#include <sysc/utils/sc_report_handler.h>

#include <type_traits>
#include <utility>

namespace sc_dt_integer_subrefs_probe
{

using Int64 = sc_dt::int64;
using UInt64 = sc_dt::uint64;
using NativeInt = int;
using NativeUInt = unsigned int;
using NativeLong = long;
using NativeULong = unsigned long;

/** @brief Expose the library-owned bounded signed range-reference pool. */
struct int_pool_access : sc_dt::sc_int_subref
{
  /** @brief Return the pool for diagnostics; callers must not delete/reset it. */
  static sc_core::sc_vpool<sc_dt::sc_int_subref>& get_pool()
  {
    return m_pool;
  }
};

/** @brief Expose the library-owned bounded unsigned range-reference pool. */
struct uint_pool_access : sc_dt::sc_uint_subref
{
  /** @brief Return the pool for diagnostics; callers must not delete/reset it. */
  static sc_core::sc_vpool<sc_dt::sc_uint_subref>& get_pool()
  {
    return m_pool;
  }
};

/** @brief Expose the library-owned arbitrary signed range-reference pool. */
struct signed_pool_access : sc_dt::sc_signed_subref
{
  /** @brief Return the pool for diagnostics; callers must not delete/reset it. */
  static sc_core::sc_vpool<sc_dt::sc_signed_subref>& get_pool()
  {
    return m_pool;
  }
};

/** @brief Expose the library-owned arbitrary unsigned range-reference pool. */
struct unsigned_pool_access : sc_dt::sc_unsigned_subref
{
  /** @brief Return the pool for diagnostics; callers must not delete/reset it. */
  static sc_core::sc_vpool<sc_dt::sc_unsigned_subref>& get_pool()
  {
    return m_pool;
  }
};

/** @brief Derive access to the protected signed range-view initializer. */
struct signed_subref_initializer : sc_dt::sc_signed_subref_r
{
  /** @brief Initialize a diagnostic shell from its live parent and range. */
  void set(const sc_dt::sc_signed* owner, int left, int right)
  {
    initialize(owner, left, right);
  }
};

/** @brief Derive access to the protected unsigned range-view initializer. */
struct unsigned_subref_initializer : sc_dt::sc_unsigned_subref_r
{
  /** @brief Initialize a diagnostic shell from its live parent and range. */
  void set(const sc_dt::sc_unsigned* owner, int left, int right)
  {
    initialize(owner, left, right);
  }
};

/** @brief Select a borrowed const range from a bounded signed owner.
 * @param owner Live parent integer, retained by the caller.
 * @param left Most-significant selected bit.
 * @param right Least-significant selected bit.
 * @return Reference to the owner's reusable const proxy shell.
 */
inline const sc_dt::sc_int_subref_r& const_int_range(const sc_dt::sc_int_base& owner, int left, int right)
{
  return owner(left, right);
}

/** @brief Select a borrowed const range from a bounded unsigned owner. */
inline const sc_dt::sc_uint_subref_r& const_uint_range(const sc_dt::sc_uint_base& owner, int left, int right)
{
  return owner(left, right);
}

/** @brief Select a borrowed const range from an arbitrary signed owner. */
inline const sc_dt::sc_signed_subref_r& const_signed_range(const sc_dt::sc_signed& owner, int left, int right)
{
  return owner(left, right);
}

/** @brief Select a borrowed const range from an arbitrary unsigned owner. */
inline const sc_dt::sc_unsigned_subref_r& const_unsigned_range(const sc_dt::sc_unsigned& owner, int left, int right)
{
  return owner(left, right);
}

/** @brief Return an owned signed range proxy made through protected initialize.
 * @param owner Live parent retained while the returned proxy is used.
 * @param left Most-significant selected bit.
 * @param right Least-significant selected bit.
 * @return Independent proxy shell borrowing the supplied owner.
 */
inline sc_dt::sc_signed_subref_r initialized_signed_range(const sc_dt::sc_signed& owner, int left, int right)
{
  signed_subref_initializer shell;
  shell.set(&owner, left, right);
  return shell;
}

/** @brief Return an owned unsigned range proxy made through protected initialize. */
inline sc_dt::sc_unsigned_subref_r initialized_unsigned_range(const sc_dt::sc_unsigned& owner, int left, int right)
{
  unsigned_subref_initializer shell;
  shell.set(&owner, left, right);
  return shell;
}

/** @brief Form a generic numeric source through SystemC concatenation.
 * @param left First source value, retained while the returned shell is read.
 * @param right Second source value, retained while the returned shell is read.
 * @return Library-owned concatenation shell implementing sc_generic_base.
 */
inline const sc_dt::sc_concatref& generic_concat_source(const sc_dt::sc_signed& left, const sc_dt::sc_unsigned& right)
{
  return (left, right);
}

/** @brief Return capacity of the bounded signed range-reference pool. */
inline std::size_t int_pool_size()
{
  return int_pool_access::get_pool().size();
}

/** @brief Return capacity of the bounded unsigned range-reference pool. */
inline std::size_t uint_pool_size()
{
  return uint_pool_access::get_pool().size();
}

/** @brief Return capacity of the arbitrary signed range-reference pool. */
inline std::size_t signed_pool_size()
{
  return signed_pool_access::get_pool().size();
}

/** @brief Return capacity of the arbitrary unsigned range-reference pool. */
inline std::size_t unsigned_pool_size()
{
  return unsigned_pool_access::get_pool().size();
}

/** @brief Make invalid range reports catchable in this isolated subprocess.
 * @return Previous process-global error actions, which must be restored.
 */
inline sc_core::sc_actions throw_errors()
{
  return sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
}

/** @brief Restore the process-global report action returned by throw_errors().
 * @param actions The previous action mask.
 */
inline void restore_errors(sc_core::sc_actions actions)
{
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, actions);
}

static_assert(std::is_copy_constructible<sc_dt::sc_int_subref>::value, "sc_int subref copy constructor");
static_assert(std::is_copy_constructible<sc_dt::sc_uint_subref>::value, "sc_uint subref copy constructor");
static_assert(std::is_copy_constructible<sc_dt::sc_signed_subref>::value, "sc_signed subref copy constructor");
static_assert(std::is_copy_constructible<sc_dt::sc_unsigned_subref>::value, "sc_unsigned subref copy constructor");
static_assert(std::is_copy_constructible<sc_dt::sc_int_subref_r>::value, "sc_int subref_r copy constructor");
static_assert(std::is_copy_constructible<sc_dt::sc_uint_subref_r>::value, "sc_uint subref_r copy constructor");
static_assert(std::is_copy_constructible<sc_dt::sc_signed_subref_r>::value, "sc_signed subref_r copy constructor");
static_assert(std::is_copy_constructible<sc_dt::sc_unsigned_subref_r>::value, "sc_unsigned subref_r copy constructor");
static_assert(!std::is_default_constructible<sc_dt::sc_int_subref>::value,
              "sc_int subref default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_uint_subref>::value,
              "sc_uint subref default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_signed_subref>::value,
              "sc_signed subref default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_unsigned_subref>::value,
              "sc_unsigned subref default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_int_subref_r>::value,
              "sc_int subref_r default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_uint_subref_r>::value,
              "sc_uint subref_r default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_signed_subref_r>::value,
              "sc_signed subref_r default constructor stays protected");
static_assert(!std::is_default_constructible<sc_dt::sc_unsigned_subref_r>::value,
              "sc_unsigned subref_r default constructor stays protected");
static_assert(std::is_same<decltype(std::declval<sc_dt::sc_int_base&>()(1, 0)), sc_dt::sc_int_subref&>::value,
              "mutable bounded signed range selection returns sc_int_subref");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_int_base&>()(1, 0)), const sc_dt::sc_int_subref_r&>::value,
  "const bounded signed range selection returns sc_int_subref_r");
static_assert(std::is_same<decltype(std::declval<sc_dt::sc_uint_base&>()(1, 0)), sc_dt::sc_uint_subref&>::value,
              "mutable bounded unsigned range selection returns sc_uint_subref");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_uint_base&>()(1, 0)), const sc_dt::sc_uint_subref_r&>::value,
  "const bounded unsigned range selection returns sc_uint_subref_r");
static_assert(std::is_same<decltype(std::declval<sc_dt::sc_signed&>()(1, 0)), sc_dt::sc_signed_subref&>::value,
              "mutable arbitrary signed range selection returns sc_signed_subref");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_signed&>()(1, 0)), const sc_dt::sc_signed_subref_r&>::value,
  "const arbitrary signed range selection returns sc_signed_subref_r");
static_assert(std::is_same<decltype(std::declval<sc_dt::sc_unsigned&>()(1, 0)), sc_dt::sc_unsigned_subref&>::value,
              "mutable arbitrary unsigned range selection returns sc_unsigned_subref");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_unsigned&>()(1, 0)), const sc_dt::sc_unsigned_subref_r&>::value,
  "const arbitrary unsigned range selection returns sc_unsigned_subref_r");

}  // namespace sc_dt_integer_subrefs_probe

#endif  // PYSYSTEMC_TEST_SC_DT_INTEGER_SUBREFS_PROBE_H
