/** @file hash_helpers.cpp
 * @brief String overloads for native void-pointer hash utilities.
 * @details Inputs are borrowed NUL-terminated strings valid for each call. No
 * callbacks or scheduler entry; call on the simulation thread with the GIL held.
 * Native allocation exceptions propagate. Duplicate keys require exactly one
 * sc_strhash_kfree call; allocation ownership transfers to the caller.
 */
#include <sysc/utils/sc_hash.h>
namespace pysystemc
{
/** @brief Hash a borrowed string without Python-to-void-pointer conversion. */
unsigned string_hash(const char* key)
{
  return sc_core::default_str_hash_fn(key);
}
/** @brief Allocate a native copy; caller must release it with sc_strhash_kfree. */
void* duplicate_string_key(const char* key)
{
  return sc_core::sc_strhash_kdup(key);
}
/** @brief Compare two borrowed NUL-terminated string keys. */
int compare_string_keys(const char* left, const char* right)
{
  return sc_core::sc_strhash_cmp(left, right);
}
/** @brief Compare a borrowed string with a borrowed native key allocation. */
int compare_string_keys(const char* left, const void* right)
{
  return sc_core::sc_strhash_cmp(left, right);
}
/** @brief Compare a borrowed native key allocation with a borrowed string. */
int compare_string_keys(const void* left, const char* right)
{
  return sc_core::sc_strhash_cmp(left, right);
}
}  // namespace pysystemc
