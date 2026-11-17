/** @file hash_callback_pointers_probe.h
 * @brief Native consumer and value checks for SystemC hash callback adapters.
 * @details The probe owns only stack-local hash tables and callback counters.
 * Callback pointers and table entries borrow their targets; no Python state is
 * retained or simulation kernel state changed.
 */
#pragma once

#include <systemc>
#include <type_traits>
#include <utility>

#include "../../src/amal/eda/systemc/sc_core/callback_pointers.h"

namespace hash_callback_pointers_probe
{
/** @brief Count calls through the exact SystemC hash callback type. */
inline unsigned hash_calls = 0;
/** @brief Count calls through the exact SystemC comparison callback type. */
inline unsigned compare_calls = 0;

/** @brief Native hash callback with SystemC's exact public signature. */
inline unsigned hash_key(const void*)
{
  ++hash_calls;
  return 0;
}

/** @brief Native comparison callback with SystemC's exact public signature. */
inline int compare_keys(const void*, const void*)
{
  ++compare_calls;
  return 0;
}

/** @brief Select deliberately wrong overloads for signature rejection checks. */
inline unsigned wrong_hash(int)
{
  return 1;
}

/** @brief Select a comparison overload with a deliberately wrong signature. */
inline int wrong_compare(int, int)
{
  return 1;
}

/** @brief Provide exact and wrong overloads for native selection checks. */
inline unsigned overloaded_hash(const void*)
{
  ++hash_calls;
  return 0;
}

/** @brief Provide an incompatible overload beside the exact hash overload. */
inline unsigned overloaded_hash(int)
{
  return 1;
}

/** @brief Provide exact and wrong comparator overloads for native selection. */
inline int overloaded_compare(const void*, const void*)
{
  ++compare_calls;
  return 0;
}

/** @brief Provide an incompatible comparator overload. */
inline int overloaded_compare(int, int)
{
  return 1;
}

static_assert(std::is_same<sc_core::sc_phash_base::hash_fn_t, unsigned (*)(const void*)>::value,
              "SystemC hash function typedef signature changed");
static_assert(std::is_same<sc_core::sc_phash_base::cmpr_fn_t, int (*)(const void*, const void*)>::value,
              "SystemC comparison typedef signature changed");
static_assert(std::is_same<pysystemc::callback_pointer<2>::pointer_type, sc_core::sc_phash_base::hash_fn_t>::value,
              "hash pointer adapter must preserve the native typedef");
static_assert(std::is_same<pysystemc::callback_pointer<0>::pointer_type, sc_core::sc_phash_base::cmpr_fn_t>::value,
              "existing CFT holder must preserve the native comparison typedef");
static_assert(std::is_same<decltype(std::declval<pysystemc::callback_pointer<2>>().get()),
                           sc_core::sc_phash_base::hash_fn_t>::value,
              "hash holder returns the exact native typedef");
static_assert(std::is_same<decltype(std::declval<pysystemc::callback_pointer<0>>().get()),
                           sc_core::sc_phash_base::cmpr_fn_t>::value,
              "comparison holder returns the exact native typedef");

/** @brief Exercise exact holder consumers and observable native table behavior.
 * @details The table borrows native callback code and stack key/content values;
 * it is destroyed before this function returns. Hold the GIL at the Python
 * boundary. The native callbacks do not enter Python or throw.
 */
inline bool exercise_configured_table(sc_core::sc_phash_base& table)
{
  hash_calls = 0;
  compare_calls = 0;
  int first_key = 1;
  int second_key = 2;
  int first_contents = 3;
  int second_contents = 4;
  const bool first_inserted = table.insert(&first_key, &first_contents) == 0;
  const bool duplicate_updated = table.insert(&second_key, &second_contents) == 1;
  return first_inserted && duplicate_updated && table.count() == 1 && table.contains(&first_key) &&
         table.contains(&second_key) && hash_calls > 0 && compare_calls > 0;
}

/** @brief Set exact native callback pointers and exercise a local native table. */
inline bool exercise_native_table(const pysystemc::callback_pointer<2>& hash,
                                  const pysystemc::callback_pointer<0>& compare)
{
  sc_core::sc_phash_base table;
  pysystemc::set_phash_hash_callback(table, hash);
  pysystemc::set_phash_compare_callback(table, compare);
  return exercise_configured_table(table);
}

/** @brief Check that holders preserve the selected overloaded function values. */
inline bool exact_hash_pointer(const pysystemc::callback_pointer<2>& pointer)
{
  sc_core::sc_phash_base::hash_fn_t selected = &overloaded_hash;
  return pointer.get() == selected;
}

/** @brief Check exact comparator function identity for an overloaded name. */
inline bool exact_compare_pointer(const pysystemc::callback_pointer<0>& pointer)
{
  sc_core::sc_phash_base::cmpr_fn_t selected = &overloaded_compare;
  return pointer.get() == selected;
}
}  // namespace hash_callback_pointers_probe
