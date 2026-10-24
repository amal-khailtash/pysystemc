/** @file sc_core_vector_creator_public.cpp
 * @brief Native creator failure rollback, context restoration and destruction.
 */
#include <iostream>
#include <systemc>

#include "sc_core_vector_creator_probe.h"

/** @brief Compare failed initialization followed by successful native ownership. */
int sc_main(int, char**)
{
  using namespace vector_creator_probe;
  {
    sc_core::sc_vector<Element> vector("creator_rollback");
    try
    {
      vector.init(4, FailingCreator{});
      return 2;
    }
    catch (const std::runtime_error& error)
    {
      if (std::string(error.what()) != "creator failure") return 3;
    }
    if (vector.size() != 0 || Element::destroyed != 2) return 4;
    vector.init(3, [](const char* name, std::size_t index) { return new Element(name, static_cast<int>(index)); });
    for (int index = 0; index < 3; ++index)
      if (vector[index].index != index) return 5;
    {
      Element outside("after_creator_failure");
      if (outside.get_parent_object()) return 6;
    }
    if (Element::destroyed != 3) return 7;
  }
  if (Element::destroyed != 6) return 8;
  std::cout << "VECTOR_CREATOR_OK 0 3 6\n";
  return 0;
}
