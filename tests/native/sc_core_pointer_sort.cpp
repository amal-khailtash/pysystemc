/** @file sc_core_pointer_sort.cpp
 * @brief Native CFT pointer comparator and sc_pvector sorting reference.
 * @details Vector borrows stack-owned pointees; sort callback owns no storage.
 */
#include <sysc/utils/sc_pvector.h>

#include <systemc>
/** @brief Compare the pointees of native vector entries. */
int compare(const void* left, const void* right)
{
  int a = **static_cast<int*const*>(left), b = **static_cast<int*const*>(right);
  return (a > b) - (a < b);
}
/** @brief Sort pointer entries with the native callback typedef. */
int sc_main(int, char**)
{
  int values[3] = {3, 1, 2};
  sc_core::sc_pvector<int*> items;
  for (auto& value : values) items.push_back(&value);
  sc_core::CFT callback = compare;
  items.sort(callback);
  if (*items[0] != 1 || *items[1] != 2 || *items[2] != 3) return 2;
  std::cout << "POINTER_SORT_REFERENCE 1 2 3\n";
  return 0;
}
