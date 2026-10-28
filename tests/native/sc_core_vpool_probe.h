/** @file sc_core_vpool_probe.h
 * @brief Typed fixtures for checking SystemC's temporary vector pool.
 * @details Internal fixtures let sc_vpool construct its native T[8] storage.
 * External fixtures own a real T[8] array declared before the pool so the pool
 * is destroyed first. Allocation results are observed by address only; tests
 * never dereference unconfigured reference values or create kernel processes.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_VPOOL_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_VPOOL_PROBE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <systemc>
#include <type_traits>
#include <utility>

namespace vpool_probe
{

inline constexpr int pool_log2 = 3;
inline constexpr std::size_t pool_capacity = std::size_t{1} << pool_log2;

/** @brief User-defined value type proving unrestricted template support. */
struct user_slot
{
  std::uint64_t payload{};
};

/** @brief Track value lifetimes for internal and caller-owned pool storage. */
struct lifetime_slot
{
  static inline unsigned constructors = 0;
  static inline unsigned destructors = 0;

  /** @brief Count construction of one real pooled value. */
  lifetime_slot()
  {
    ++constructors;
  }
  /** @brief Count destruction when the real array owner is released. */
  ~lifetime_slot()
  {
    ++destructors;
  }
};

/** @brief Own a native default-allocated sc_vpool<T> and expose it by reference. */
template <class T>
class allocated_pool
{
  sc_core::sc_vpool<T> pool_;

 public:
  /** @brief Allocate native T[8] storage using sc_vpool's default pool pointer. */
  allocated_pool() : pool_(pool_log2)
  {
  }

  /** @brief Return the owned native pool; this fixture must outlive the reference. */
  sc_core::sc_vpool<T>& pool()
  {
    return pool_;
  }
};

/** @brief Own a real T[8] array and pass it to sc_vpool as external storage. */
template <class T>
class external_pool
{
  std::array<T, pool_capacity> storage_{};
  sc_core::sc_vpool<T> pool_;

 public:
  /** @brief Construct values before the pool, which is destroyed before them. */
  external_pool() : pool_(pool_log2, storage_.data())
  {
  }

  /** @brief Return the pool borrowing the array owned by this fixture. */
  sc_core::sc_vpool<T>& pool()
  {
    return pool_;
  }
};

/** @brief Return the element index between two pointers into one native T array. */
template <class T>
std::ptrdiff_t slot_index(T* base, T* slot)
{
  return slot - base;
}

/** @brief Check alignment of a typed pointer without reading its value. */
template <class T>
bool slot_aligned(T* slot)
{
  return reinterpret_cast<std::uintptr_t>(slot) % alignof(T) == 0;
}

/** @brief Return the native byte stride of one T array element. */
template <class T>
std::size_t slot_stride()
{
  return sizeof(T);
}

/** @brief Return native alignment of one T array element. */
template <class T>
std::size_t slot_alignment()
{
  return alignof(T);
}

/** @brief Reset lifetime counters before an isolated pool-construction case. */
inline void reset_lifetime_counts()
{
  lifetime_slot::constructors = 0;
  lifetime_slot::destructors = 0;
}

/** @brief Return constructors invoked for tracked values. */
inline unsigned lifetime_constructor_count()
{
  return lifetime_slot::constructors;
}

/** @brief Return destructors invoked for tracked values. */
inline unsigned lifetime_destructor_count()
{
  return lifetime_slot::destructors;
}

static_assert(
  std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_unsigned>&>().allocate()), sc_dt::sc_unsigned*>);
static_assert(
  std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_concatref>&>().allocate()), sc_dt::sc_concatref*>);
static_assert(std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_unsigned_bitref>&>().allocate()),
                             sc_dt::sc_unsigned_bitref*>);
static_assert(std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_uint_subref>&>().allocate()),
                             sc_dt::sc_uint_subref*>);
static_assert(std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_signed_bitref>&>().allocate()),
                             sc_dt::sc_signed_bitref*>);
static_assert(
  std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_int_bitref>&>().allocate()), sc_dt::sc_int_bitref*>);
static_assert(
  std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_int_subref>&>().allocate()), sc_dt::sc_int_subref*>);
static_assert(std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_signed_subref>&>().allocate()),
                             sc_dt::sc_signed_subref*>);
static_assert(std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_uint_bitref>&>().allocate()),
                             sc_dt::sc_uint_bitref*>);
static_assert(std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_concat_bool>&>().allocate()),
                             sc_dt::sc_concat_bool*>);
static_assert(std::is_same_v<decltype(std::declval<sc_core::sc_vpool<sc_dt::sc_unsigned_subref>&>().allocate()),
                             sc_dt::sc_unsigned_subref*>);
static_assert(std::is_same_v<decltype(std::declval<sc_core::sc_vpool<user_slot>&>().allocate()), user_slot*>);

}  // namespace vpool_probe

#endif  // PYSYSTEMC_TEST_SC_CORE_VPOOL_PROBE_H
