/** @file sc_core_phash_public.cpp
 * @brief Independent native generic/specialized pointer hash declaration matrix.
 */
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "sc_core_phash_public_probe.h"

/** @brief Enforce native reference assertions in release and debug builds. */
void require(bool condition)
{
  if (!condition) throw std::runtime_error("native phash reference mismatch");
}

/** @brief Exercise all public table overloads and iterator declarations.
 * @tparam Value Live borrowed content type, unrestricted by the binding.
 * @details Explicit duplicated keys are released exactly once. Table copies
 * borrow content objects; value storage lasts for this process. No scheduler or
 * Python callback is invoked by this native reference.
 */
template <class Value>
void exercise(const char* label)
{
  using namespace phash_public_probe;
  using F = fixture<Value>;
  using T = typename F::table_type;
  using I = typename F::iterator_type;
  T empty;
  require(empty.empty());
  F slots;
  T table(F::value(0), 11, 2, 2.0, false, hash_key, compare_key);
  std::vector<int> inserted{table.insert(key(1)), table.insert(key(2), F::value(1)), table.insert(key(2), F::value(2)),
                            table.insert_if_not_exists(key(2), F::value(0)), table.insert_if_not_exists(key(3))};
  require(inserted == std::vector<int>({0, 0, 1, 1, 0}));
  require(table.lookup(key(2), slots.content_slot()) == 1 && slots.output_index() == 2);
  require(table.lookup(key(7), slots.content_slot()) == 0 && slots.output_index() == 0);
  require(table.lookup(key(2), nullptr) == 1 && table.lookup(key(7), nullptr) == 0);
  require(table.contains(key(2)) && !table.contains(key(7)));
  require(F::value_index(table[key(1)]) == 0 && F::value_index(table[key(7)]) == 0);
  I iterator(&table);
  std::vector<int> collected;
  while (!iterator.empty())
  {
    int number = key_value(iterator.key());
    collected.push_back(number);
    if (number == 1) require(F::value_index(iterator.set_contents(F::value(1))) == 1);
    require(F::value_index(iterator.contents()) >= 0);
    iterator.step();
  }
  std::sort(collected.begin(), collected.end());
  require(collected == std::vector<int>({1, 2, 3}));
  iterator.reset(table);
  require(!iterator.empty());
  iterator.reset(&table);
  require(!iterator.empty());
  I reference(table);
  require(!reference.empty());
  int removed = table.remove(key(2), slots.key_slot(), slots.content_slot());
  require(removed == 1 && slots.output_key() == 2 && slots.output_index() == 2);
  require(table.remove(key(2), slots.key_slot(), slots.content_slot()) == 0 && slots.output_key() == -1 &&
          slots.output_index() == -1);
  require(table.remove(key(3)) == 1 && table.count() == 1);
  T pointer_copy, reference_copy;
  pointer_copy.copy(&table);
  reference_copy.copy(table);
  require(pointer_copy.count() == 1 && reference_copy.count() == 1);
  require(F::value_index(pointer_copy[key(1)]) == 1 && F::value_index(reference_copy[key(1)]) == 1);
  std::cout << "PHASH_BORROWED " << label;
  for (int value : inserted) std::cout << ' ' << value;
  std::cout << ' ' << removed << ' ' << collected.size() << ' ' << table.count() << ' ' << pointer_copy.count() << ' '
            << reference_copy.count() << '\n';
  table.erase();
  pointer_copy.erase();
  reference_copy.erase();
  table.insert(key(1), F::value(1));
  table.insert(key(2), F::value(1));
  table.insert(key(3), F::value(2));
  int direct_removed = table.remove_by_contents(F::value(1));
  int predicate_removed = table.remove_by_contents(same_content, F::argument(2));
  require(direct_removed == 2 && predicate_removed == 1 && table.empty());
  std::cout << "PHASH_CONTENTS " << label << ' ' << direct_removed << ' ' << predicate_removed << ' ' << table.count()
            << '\n';
  reset_counts();
  T owned(F::value(0), 11, 2, 2.0, false, hash_key, compare_key),
    copied(F::value(0), 11, 2, 2.0, false, hash_key, compare_key);
  require(owned.insert(key(4), F::value(1), duplicate_key) == 0);
  require(owned.insert_if_not_exists(key(4), F::value(0), duplicate_key) == 1);
  require(owned.insert_if_not_exists(key(5), F::value(2), duplicate_key) == 0);
  require(owned.remove(key(4), free_key) == 1);
  owned.insert(key(4), F::value(1), duplicate_key);
  copied.insert(key(6), F::value(0), duplicate_key);
  copied.copy(owned, duplicate_key, free_key);
  copied.copy(owned, duplicate_key, free_key);
  require(copied.count() == 2 && F::value_index(copied[key(4)]) == 1);
  require(copied.remove_by_contents(F::argument(1), free_key) == 1);
  require(copied.remove_by_contents(same_content, F::argument(2), free_key) == 1);
  owned.erase(free_key);
  copied.erase(free_key);
  require(duplicates == 8 && frees == 8);
  std::cout << "PHASH_OWNERSHIP " << label << ' ' << duplicates << ' ' << frees << ' ' << owned.count() << ' '
            << copied.count() << '\n';
}

/** @brief Cover arbitrary content types and all three inventoried parameter types. */
int sc_main(int, char**)
{
  exercise<phash_public_probe::payload>("custom");
  exercise<sc_dt::sc_length_param>("length");
  exercise<sc_dt::sc_fxtype_params>("fixed");
  exercise<sc_dt::sc_fxcast_switch>("cast");
  return 0;
}
