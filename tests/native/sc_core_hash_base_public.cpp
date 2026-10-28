/** @file sc_core_hash_base_public.cpp
 * @brief Native reference for the public sc_phash_base and iterator families.
 * @details Tables own their bin arrays and nodes, while ordinary keys and
 * contents remain borrowed. Duplicating overloads own keys produced by the
 * probe callback and release them through the paired free callback. All
 * callbacks run synchronously on this thread and use native functions only.
 */
#include <array>
#include <iostream>
#include <set>
#include <stdexcept>
#include <systemc>
#include <type_traits>

#include "sc_core_hash_base_probe.h"

namespace
{

/** @brief Stop the native reference on the first mismatched observation. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

}  // namespace

/** @brief Exercise every public base and base-iterator operation in native C++. */
int sc_main(int, char**)
{
  using namespace sc_core_hash_base_probe;
  using sc_core::sc_phash_base;
  using sc_core::sc_phash_base_iter;
  static_assert(std::is_same_v<sc_phash_base::hash_fn_t, unsigned (*)(const void*)>);
  static_assert(std::is_same_v<sc_phash_base::cmpr_fn_t, int (*)(const void*, const void*)>);
  static_assert(std::is_class_v<sc_core::sc_phash_elem>);

  sc_phash_base default_table;
  verify(default_table.empty() && default_table.count() == 0, "default constructor and initial state");
  BaseProbe normalized(nullptr, 4, 2, 2.0, false, hash_int_value, compare_int_value);
  verify(normalized.bin_count() == 5, "even constructor size normalization");
  verify(normalized.find_node(key(21)) == nullptr, "derived lookup reports missing node");
  normalized.insert(key(21), contents(22));
  auto* borrowed_element = normalized.find_node(key(21));
  verify(borrowed_element != nullptr && !is_null_element(borrowed_element), "derived lookup returns borrowed node");
  verify(normalized.find_node(key(20)) == nullptr, "derived lookup returns null for absent node");
  normalized.erase();
  BaseProbe fallback(nullptr, 0, 2, 2.0, false, hash_int_value, compare_int_value);
  verify(fallback.bin_count() == 11, "nonpositive constructor size normalization");

  sc_phash_base null_table;
  null_table.set_hash_fn(hash_int_value);
  null_table.set_cmpr_fn(compare_int_value);
  verify(null_table.insert(nullptr, nullptr) == 0 && null_table.contains(nullptr), "null key and contents insertion");
  verify(null_table.lookup(nullptr, contents_output_slot()) == 1 && contents_output() == nullptr,
         "null contents lookup retains found status");
  verify(null_table[nullptr] == nullptr && null_table.remove(nullptr) == 1 && null_table.empty(),
         "null key access and removal");

  sc_phash_base table(contents(9), 4, 2, 2.0, false, hash_int_value, compare_int_value);
  verify(table.empty() && table.count() == 0, "configured table initial state");
  verify(table.insert(key(1)) == 0, "default-content insert");
  verify(table.insert(key(2), contents(2)) == 0, "key-content insert");
  verify(table.insert(key(2), contents(3)) == 1, "insert updates duplicate");
  verify(table.insert_if_not_exists(key(2), contents(8)) == 1, "insert-if-missing preserves duplicate");
  verify(table.insert_if_not_exists(key(3)) == 0, "insert-if-missing uses default contents");
  verify(table.count() == 3 && !table.empty(), "insert count and empty state");
  verify(table.contains(key(1)) && !table.contains(key(20)), "contains hit and miss");
  verify(read_int(table[key(1)]) == 9 && read_int(table[key(20)]) == 9, "operator lookup default");

  verify(table.lookup(key(2), contents_output_slot()) == 1, "lookup with output pointer");
  verify(read_int(contents_output()) == 3, "lookup output contents");
  verify(table.lookup(key(2), nullptr) == 1, "lookup with null output pointer");
  verify(table.lookup(key(20), contents_output_slot()) == 0 && read_int(contents_output()) == 9,
         "missing lookup reports default contents");

  sc_phash_base_iter iterator(&table);
  verify(!iterator.empty(), "pointer iterator constructor");
  std::set<int> iterated_keys;
  while (!iterator.empty())
  {
    iterated_keys.insert(read_int(iterator.key()));
    if (read_int(iterator.key()) == 1)
      verify(read_int(iterator.set_contents(contents(10))) == 10, "iterator contents assignment");
    iterator++;
  }
  verify(iterated_keys == std::set<int>{1, 2, 3}, "iterator key traversal");
  iterator.reset(table);
  verify(!iterator.empty(), "reference iterator reset");
  iterator.remove();
  verify(table.count() == 2, "iterator removal");
  iterator.reset(&table);
  int iterator_free_removed = 0;
  while (!iterator.empty())
  {
    if (read_int(iterator.key()) == 2)
    {
      verify(read_int(iterator.contents()) == 3, "iterator content read");
      iterator.remove();
      ++iterator_free_removed;
      break;
    }
    iterator.step();
  }
  verify(iterator_free_removed == 1 && table.count() == 1, "iterator step and removal");

  verify(table.insert(key(4), contents(4)) == 0, "contents-removal insert");
  verify(table.remove_by_contents(contents(4)) == 1, "remove by contents");
  verify(table.insert(key(5), contents(5)) == 0 && table.insert(key(6), contents(6)) == 0, "predicate-removal inserts");
  int predicate_arg = 5;
  auto predicate = [](const void* value, void* arg) -> bool { return read_int(value) == *static_cast<int*>(arg); };
  verify(table.remove_by_contents(predicate, &predicate_arg) == 1, "remove by contents predicate");
  verify(table.remove(key(5)) == 0, "remove absent key");
  verify(table.remove(key(6)) == 1, "remove existing key");

  sc_phash_base output_table(contents(9), 11, 5, 2.0, true, hash_int_value, compare_int_value);
  output_table.insert(key(7), contents(8));
  verify(output_table.remove(key(7), key_output_slot(), contents_output_slot()) == 1, "remove with output pointers");
  verify(read_int(key_output()) == 7 && read_int(contents_output()) == 8, "remove output pointers contents");
  verify(output_table.remove(key(7), key_output_slot(), contents_output_slot()) == 0, "missing output removal");
  verify(key_output() == nullptr && contents_output() == nullptr, "missing output removal clears slots");

  sc_phash_base source(contents(9), 11, 5, 2.0, true, hash_int_value, compare_int_value);
  source.insert(key(11), contents(12));
  source.insert(key(12), contents(13));
  sc_phash_base pointer_copy;
  pointer_copy.copy(&source);
  verify(pointer_copy.count() == 2 && pointer_copy[key(11)] == source[key(11)], "pointer copy shares pointers");
  sc_phash_base reference_copy;
  reference_copy.copy(source);
  verify(reference_copy.count() == 2 && reference_copy[key(12)] == source[key(12)], "reference copy shares pointers");

  sc_phash_base owned(contents(9), 11, 5, 2.0, true, hash_int_value, compare_int_value);
  verify(owned.insert(key(14), contents(15), duplicate_key) == 0, "duplicate-key insert");
  verify(owned.insert_if_not_exists(key(14), contents(16), duplicate_key) == 1, "duplicate-key insert-if-missing hit");
  verify(owned.insert_if_not_exists(key(15), contents(16), duplicate_key) == 0, "duplicate-key insert-if-missing");
  sc_phash_base duplicate_copy(nullptr, 11, 5, 2.0, true, hash_int_value, compare_int_value);
  duplicate_copy.copy(owned, duplicate_key, free_key);
  verify(duplicate_copy.count() == 2 && read_int(duplicate_copy[key(14)]) == 15, "callback copy duplicates keys");
  duplicate_copy.copy(owned, duplicate_key, free_key);
  verify(freed_keys() == 2 && duplicate_copy.count() == 2, "callback copy frees prior owned keys");
  sc_phash_base_iter owned_iterator(duplicate_copy);
  verify(!owned_iterator.empty(), "reference iterator constructor");
  owned_iterator.remove(free_key);
  verify(duplicate_copy.count() == 1, "iterator removal frees owned key");
  verify(duplicate_copy.remove(key(15), free_key) == 1, "remove frees owned key");
  verify(freed_keys() == 4 && duplicate_copy.empty(), "callback removals release owned keys");
  owned.insert(key(16), contents(17), duplicate_key);
  owned.insert(key(17), contents(16), duplicate_key);
  duplicate_copy.copy(owned, duplicate_key, free_key);
  verify(duplicate_copy.remove_by_contents(contents(16), free_key) == 2, "remove-by-contents frees owned keys");
  predicate_arg = 17;
  verify(duplicate_copy.remove_by_contents(predicate, &predicate_arg, free_key) == 1,
         "predicate removal frees owned key");
  duplicate_copy.erase(free_key);
  verify(freed_keys() == 8 && duplicate_copy.empty(), "callback erase releases owned keys");
  owned.erase(free_key);
  verify(freed_keys() == 12 && owned.empty(), "owned table erase releases duplicated keys");
  table.erase();
  source.erase();
  pointer_copy.erase();
  reference_copy.erase();
  output_table.erase();

  auto* element = static_cast<sc_core::sc_phash_elem*>(nullptr);
  verify(is_null_element(element), "forward-declared element pointer is borrowed and null");

  std::cout
    << "SC_HASH_BASE_REFERENCE rows=45 counts=0,3,2 copies=2,2 iterator=3 removed=6 freed=12 bins=5,11 elem=null\n";
  return 0;
}
