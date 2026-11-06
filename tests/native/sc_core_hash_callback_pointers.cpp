/** @file sc_core_hash_callback_pointers.cpp
 * @brief Native reference for exact sc_phash_base callback pointer values.
 * @details The program constructs independent table state on the native kernel
 * process, consumes exact native pointers, and returns after all borrowed stack
 * keys and callback users finish. No Python callbacks or SystemC simulation are
 * used.
 */
#include <iostream>
#include <stdexcept>

#include "hash_callback_pointers_probe.h"

/** @brief Compare exact callback-holder table behavior with the Python binding. */
int sc_main(int, char**)
{
  using hash_callback_pointers_probe::exact_compare_pointer;
  using hash_callback_pointers_probe::exact_hash_pointer;
  using hash_callback_pointers_probe::exercise_native_table;
  const auto hash = pysystemc::make_callback_pointer<2, &hash_callback_pointers_probe::overloaded_hash>();
  const auto compare = pysystemc::make_callback_pointer<0, &hash_callback_pointers_probe::overloaded_compare>();
  if (!exact_hash_pointer(hash) || !exact_compare_pointer(compare))
  {
    throw std::runtime_error("selected callback pointer identity differs from native overload");
  }
  if (!exercise_native_table(hash, compare))
  {
    throw std::runtime_error("native callback holder did not preserve hash table behavior");
  }
  std::cout << "SC_HASH_CALLBACK_POINTERS exact-overloads table-insert duplicate-update count=1\n";
  return 0;
}
