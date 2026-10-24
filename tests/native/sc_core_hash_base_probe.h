/** @file sc_core_hash_base_probe.h
 * @brief Native helpers for auditing the public chained-hash base API.
 * @details Returned key and contents pointers borrow process-lifetime storage.
 * The duplicate/free helpers exercise the table's explicit owned-key path.
 * Python callbacks passed into a hash table are called synchronously on the
 * calling thread; cppyy must retain callback objects while the table stores
 * their function pointers. No helper releases the GIL or throws across C APIs.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_HASH_BASE_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_HASH_BASE_PROBE_H

#include <sysc/utils/sc_hash.h>

namespace sc_core_hash_base_probe
{

/** @brief Borrow an integer key with the requested semantic value.
 * @param value Integer value in the fixed probe range [0, 31].
 * @return Borrowed pointer to process-lifetime key storage.
 */
inline void* key(int value)
{
  static int values[32]{};
  values[value] = value;
  return &values[value];
}

/** @brief Borrow an integer contents object with the requested value.
 * @param value Integer value in the fixed probe range [0, 31].
 * @return Borrowed pointer to process-lifetime contents storage.
 */
inline void* contents(int value)
{
  static int values[32]{};
  values[value] = value;
  return &values[value];
}

/** @brief Read an integer through a borrowed opaque pointer.
 * @param pointer Borrowed integer pointer, which may be null.
 * @return The pointed value, or -1 for a null pointer.
 */
inline int read_int(const void* pointer)
{
  return pointer == nullptr ? -1 : *static_cast<const int*>(pointer);
}

/** @brief Compare opaque pointers without taking ownership or dereferencing.
 * @param left First borrowed pointer.
 * @param right Second borrowed pointer.
 * @return True when the pointer values are identical.
 */
inline bool same_pointer(const void* left, const void* right)
{
  return left == right;
}

/** @brief Return a null value with the opaque void pointer type. */
inline void* null_pointer()
{
  return nullptr;
}

/** @brief Hash keys by integer value, with a stable null-key hash.
 * @param pointer Borrowed key pointer, possibly null.
 * @return Unsigned value hash.
 */
inline unsigned hash_int_value(const void* pointer)
{
  return pointer == nullptr ? 0U : static_cast<unsigned>(*static_cast<const int*>(pointer));
}

/** @brief Compare borrowed keys by integer value, including null keys.
 * @param left First borrowed key pointer.
 * @param right Second borrowed key pointer.
 * @return Negative, zero or positive according to key values.
 */
inline int compare_int_value(const void* left, const void* right)
{
  const int lhs = read_int(left);
  const int rhs = read_int(right);
  return (lhs > rhs) - (lhs < rhs);
}

/** @brief Duplicate a borrowed key into table-owned heap storage.
 * @param pointer Borrowed key pointer, possibly null.
 * @return Newly allocated key pointer owned by the receiving table.
 */
inline void* duplicate_key(const void* pointer)
{
  if (pointer == nullptr) return nullptr;
  return new int(*static_cast<const int*>(pointer));
}

/** @brief Return the cumulative number of duplicated keys released. */
inline int& freed_key_count()
{
  static int count = 0;
  return count;
}

/** @brief Release a key allocated by duplicate_key and count the release.
 * @param pointer Owned duplicated key, possibly null.
 */
inline void free_key(void* pointer)
{
  if (pointer != nullptr)
  {
    delete static_cast<int*>(pointer);
    ++freed_key_count();
  }
}

/** @brief Return the cumulative number of duplicated keys released. */
inline int freed_keys()
{
  return freed_key_count();
}

/** @brief Storage backing the key output slot. */
inline void*& key_output_storage()
{
  static void* output = nullptr;
  return output;
}

/** @brief Storage backing the contents output slot. */
inline void*& contents_output_storage()
{
  static void* output = nullptr;
  return output;
}

/** @brief Return an addressable output slot for remove and lookup probes.
 * @return Borrowed address of a process-lifetime void pointer slot.
 */
inline void** key_output_slot()
{
  key_output_storage() = nullptr;
  return &key_output_storage();
}

/** @brief Return an addressable contents output slot for remove and lookup probes.
 * @return Borrowed address of a process-lifetime void pointer slot.
 */
inline void** contents_output_slot()
{
  contents_output_storage() = nullptr;
  return &contents_output_storage();
}

/** @brief Read the current value of the key output slot. */
inline void* key_output()
{
  return key_output_storage();
}

/** @brief Read the current value of the contents output slot. */
inline void* contents_output()
{
  return contents_output_storage();
}

/** @brief Report whether a public forward-declared hash node pointer is null.
 * @param element Borrowed node pointer; no node storage is accessed.
 * @return True if element is null.
 */
inline bool is_null_element(sc_core::sc_phash_elem* element)
{
  return element == nullptr;
}

/** @brief Return a null pointer of the forward-declared node type. */
inline sc_core::sc_phash_elem* null_element()
{
  return nullptr;
}

/** @brief Derived access probe for constructor normalization of protected bins.
 * @details The hash table owns nodes and its bin array; supplied key and
 * contents pointers are borrowed. Callback pointers must outlive this object.
 */
class BaseProbe final : public sc_core::sc_phash_base
{
 public:
  /** @brief Construct a native base table with caller-selected parameters. */
  BaseProbe(void* def, int size, int density, double grow, bool reorder, hash_fn_t hash_fn, cmpr_fn_t cmpr_fn)
    : sc_phash_base(def, size, density, grow, reorder, hash_fn, cmpr_fn)
  {
  }

  /** @brief Read protected bin count through a native-derived test probe. */
  int bin_count() const
  {
    return num_bins;
  }

  /** @brief Return a borrowed node pointer found through the protected base lookup.
   * @param key_value Borrowed key pointer.
   * @return Borrowed node owned by this table, or null if absent.
   */
  sc_core::sc_phash_elem* find_node(void* key_value)
  {
    return find_entry(do_hash(key_value), key_value);
  }
};

}  // namespace sc_core_hash_base_probe

#endif  // PYSYSTEMC_TEST_SC_CORE_HASH_BASE_PROBE_H
