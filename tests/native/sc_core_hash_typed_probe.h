/** @file sc_core_hash_typed_probe.h
 * @brief Small adapters for exercising typed SystemC hash templates from cppyy.
 * @details The probe owns duplicated PD keys and releases them through native
 * callbacks. Pointer-valued contents are opaque and never dereferenced. Python
 * calls run with the GIL held; the callbacks are native and do not enter Python.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_HASH_TYPED_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_HASH_TYPED_PROBE_H

#include <sysc/utils/sc_hash.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>

namespace hash_typed_probe
{

using pd_hash = sc_core::sc_pdhash<char*, int*>;
using pd_iter = sc_core::sc_pdhash_iter<char*, int*>;
using str_hash = sc_core::sc_strhash<int*>;
using str_iter = sc_core::sc_strhash_iter<int*>;
static_assert(std::is_same_v<pd_hash::iterator, pd_iter>);
static_assert(std::is_same_v<str_hash::iterator, str_iter>);

/** @brief Native slots for typed lookup and remove overloads with output pointers. */
struct pd_slots
{
  char* key = nullptr;
  int* contents = nullptr;
};

/** @brief Native slots for string lookup and remove overloads with output pointers. */
struct str_slots
{
  char* key = nullptr;
  int* contents = nullptr;
};

inline unsigned duplicate_count = 0;
inline unsigned free_count = 0;

/** @brief Duplicate a NUL-terminated key for PD-table ownership. */
inline void* duplicate_key(const void* key)
{
  const auto* text = static_cast<const char*>(key);
  const std::size_t size = std::strlen(text) + 1;
  auto* result = static_cast<char*>(std::malloc(size));
  if (!result) throw std::bad_alloc();
  std::memcpy(result, text, size);
  ++duplicate_count;
  return result;
}

/** @brief Release a key allocated by duplicate_key and count the release. */
inline void free_key(void* key)
{
  std::free(key);
  ++free_count;
}

/** @brief Configure typed PD storage with native string equality and ownership. */
class owned_pd_hash : public pd_hash
{
 public:
  /** @brief Create an empty table; key copies are owned until remove or erase. */
  owned_pd_hash()
    : pd_hash(nullptr, 11, 5, 2.0, true, sc_core::default_str_hash_fn, sc_core::sc_strhash_cmp, duplicate_key, free_key)
  {
  }
};

/** @brief Call the typed PD lookup overload with a native output slot. */
inline int lookup_pd(pd_hash& table, const char* key, pd_slots& output)
{
  return table.lookup(const_cast<char*>(key), &output.contents);
}

/** @brief Call the typed PD remove overload that transfers key and contents out. */
inline int remove_pd(pd_hash& table, const char* key, pd_slots& output)
{
  return table.remove(const_cast<char*>(key), &output.key, &output.contents);
}

/** @brief Call the string lookup overload with a native output slot. */
inline int lookup_string(str_hash& table, const char* key, str_slots& output)
{
  return table.lookup(key, &output.contents);
}

/** @brief Call the string remove overload that transfers key and contents out. */
inline int remove_string(str_hash& table, const char* key, str_slots& output)
{
  return table.remove(key, &output.key, &output.contents);
}

/** @brief Release a PD output key transferred by remove(K,K*,C*). */
inline void free_pd_output_key(pd_slots& output)
{
  free_key(output.key);
  output.key = nullptr;
}

/** @brief Release a string output key transferred by remove(const char*,char**,C*). */
inline void free_string_output_key(str_slots& output)
{
  sc_core::sc_strhash_kfree(output.key);
  output.key = nullptr;
}

/** @brief Match contents by pointer identity for remove_by_contents tests. */
inline bool content_matches(const void* contents, void* expected)
{
  return contents == expected;
}

/** @brief Reset callback counters between isolated probe phases. */
inline void reset_counts()
{
  duplicate_count = 0;
  free_count = 0;
}

/** @brief Insert a const input string through the table's historical char* API. */
inline int insert_string(str_hash& table, const char* key, int* contents)
{
  return table.insert(const_cast<char*>(key), contents);
}

/** @brief Insert only when a const input string is absent. */
inline int insert_string_if_absent(str_hash& table, const char* key, int* contents)
{
  return table.insert_if_not_exists(const_cast<char*>(key), contents);
}

/** @brief Copy a typed iterator key before its table can be changed or destroyed. */
inline std::string key_text(const char* key)
{
  return key ? key : "";
}

/** @brief Expose pointer contents as an integer for stable reference output. */
inline std::uintptr_t pointer_value(const void* pointer)
{
  return reinterpret_cast<std::uintptr_t>(pointer);
}

/** @brief Expose typed contents as an integer, including a null pointer. */
inline std::uintptr_t content_value(int* pointer)
{
  return reinterpret_cast<std::uintptr_t>(pointer);
}

/** @brief Convert a stable integer token to an opaque int pointer. */
inline int* content_pointer(std::uintptr_t value)
{
  return reinterpret_cast<int*>(value);
}

}  // namespace hash_typed_probe

#endif  // PYSYSTEMC_TEST_SC_CORE_HASH_TYPED_PROBE_H
