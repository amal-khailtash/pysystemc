/** @file sc_core_name_generator.cpp
 * @brief Independent unique-name generation state and error reference.
 * @details Native generator owns its map/buffer; results are copied before next call.
 */
#include <sysc/kernel/sc_name_gen.h>

#include <string>
#include <systemc>
#include <vector>
/** @brief Verify independent counters and recover after an invalid base name. */
int sc_main(int, char**)
{
  sc_core::sc_name_gen names, other;
  std::vector<std::string> results{names.gen_unique_name("item"), names.gen_unique_name("item"),
                                   names.gen_unique_name("first", true), names.gen_unique_name("first", true),
                                   other.gen_unique_name("item")};
  try
  {
    names.gen_unique_name("");
    return 2;
  }
  catch (const sc_core::sc_report&)
  {
  }
  std::string next = names.gen_unique_name("item");
  if (results != std::vector<std::string>{"item_0", "item_1", "first", "first_1", "item_0"} || next != "item_2")
    return 3;
  std::cout << "NAME_GENERATOR_REFERENCE";
  for (const auto& result : results) std::cout << " " << result;
  std::cout << " " << next << "\n";
  return 0;
}
