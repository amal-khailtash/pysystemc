/** @file sc_core_phash_public_probe.h
 * @brief Typed storage and callback fixtures for arbitrary native hash tables.
 * @details Values and original keys borrow process-lifetime storage. Explicit
 * duplicate/free callbacks own cloned keys, never contents. Python calls hold
 * the GIL; native callbacks neither enter Python nor switch the kernel.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_PHASH_PUBLIC_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_PHASH_PUBLIC_PROBE_H
#include <sysc/datatypes/fx/sc_fxcast_switch.h>
#include <sysc/datatypes/fx/sc_fxtype_params.h>
#include <sysc/datatypes/int/sc_length_param.h>

#include <systemc>
#include <type_traits>

namespace phash_public_probe
{
/** @brief An unrestricted user-defined borrowed content type. */
struct payload
{
  int value;
};

/** @brief Borrow three live parameter objects for one native content type. */
template <class Value>
struct value_bank;
/** @brief Stable user-value storage, independent of the simulation kernel. */
template <>
struct value_bank<payload>
{
  inline static payload values[3]{{101}, {202}, {303}};
};
/** @brief Stable arbitrary-width signed/unsigned datatype length parameters. */
template <>
struct value_bank<sc_dt::sc_length_param>
{
  inline static sc_dt::sc_length_param values[3]{sc_dt::sc_length_param(19), sc_dt::sc_length_param(137),
                                                 sc_dt::sc_length_param(257)};
};
/** @brief Stable fixed-point width/integer-width parameters. */
template <>
struct value_bank<sc_dt::sc_fxtype_params>
{
  inline static sc_dt::sc_fxtype_params values[3]{sc_dt::sc_fxtype_params(19, 7), sc_dt::sc_fxtype_params(137, 41),
                                                  sc_dt::sc_fxtype_params(257, 97)};
};
/** @brief Stable cast switches; repeated values still have distinct addresses. */
template <>
struct value_bank<sc_dt::sc_fxcast_switch>
{
  inline static sc_dt::sc_fxcast_switch values[3]{sc_dt::sc_fxcast_switch(sc_dt::SC_ON),
                                                  sc_dt::sc_fxcast_switch(sc_dt::SC_OFF),
                                                  sc_dt::sc_fxcast_switch(sc_dt::SC_ON)};
};

inline unsigned duplicates = 0;
inline unsigned frees = 0;
/** @brief Borrow a stable integer key; keys remain alive for every table. */
inline void* key(unsigned index)
{
  static int storage[16]{};
  storage[index] = index;
  return &storage[index];
}
/** @brief Copy the value of a live key; null means no removed key. */
inline int key_value(const void* pointer)
{
  return pointer ? *static_cast<const int*>(pointer) : -1;
}
/** @brief Hash keys by their value, allowing cloned-key lookup. */
inline unsigned hash_key(const void* pointer)
{
  return pointer ? *static_cast<const int*>(pointer) : 0;
}
/** @brief Compare original or duplicated integer keys by value. */
inline int compare_key(const void* left, const void* right)
{
  return key_value(left) - key_value(right);
}
/** @brief Clone a key; ownership passes to the explicitly managed table. */
inline void* duplicate_key(const void* pointer)
{
  auto* result = new int(key_value(pointer));
  ++duplicates;
  return result;
}
/** @brief Release one cloned key, with exactly one call per allocation. */
inline void free_key(void* pointer)
{
  delete static_cast<int*>(pointer);
  ++frees;
}
/** @brief Match a borrowed content pointer against a borrowed predicate argument. */
inline bool same_content(const void* content, void* argument)
{
  return content == argument;
}
/** @brief Reset test counters only after all cloned keys have been released. */
inline void reset_counts()
{
  duplicates = frees = 0;
}

/** @brief Addressable native slots for each unrestricted const-content pointer type. */
template <class Value>
struct fixture
{
  using content_type = const Value*;
  using table_type = sc_core::sc_phash<void*, content_type>;
  using iterator_type = sc_core::sc_phash_iter<void*, content_type>;
  static_assert(std::is_same_v<typename table_type::iterator, iterator_type>);
  void* removed_key = nullptr;
  content_type output = nullptr;
  /** @brief Borrow a content object without transferring its ownership. */
  static content_type value(unsigned index)
  {
    return &value_bank<Value>::values[index];
  }
  /** @brief Identify exact content pointers, including null output on failed removal. */
  static int value_index(content_type pointer)
  {
    for (unsigned i = 0; i < 3; ++i)
      if (pointer == value(i)) return i;
    return -1;
  }
  /** @brief Borrow the same content pointer through the predicate's void* parameter. */
  static void* argument(unsigned index)
  {
    return const_cast<Value*>(value(index));
  }
  /** @brief Identify the contents written through a native output pointer slot. */
  int output_index() const
  {
    return value_index(output);
  }
  /** @brief Copy the key written through the native output pointer slot. */
  int output_key() const
  {
    return key_value(removed_key);
  }
  /** @brief Borrow an output pointer slot for lookup/removal; caller owns this fixture. */
  content_type* content_slot()
  {
    return &output;
  }
  /** @brief Borrow an output key slot for removal; caller owns this fixture. */
  void** key_slot()
  {
    return &removed_key;
  }
};
}  // namespace phash_public_probe
#endif
