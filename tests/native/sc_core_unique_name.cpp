/** @file sc_core_unique_name.cpp
 * @brief Native module/process/global unique-name scope reference.
 * @details Copy native temporary strings immediately. No Python callbacks.
 */
#include <algorithm>
#include <systemc>
#include <vector>
std::vector<std::string> records;
/** @brief Check scope-local counters, preserve-first and exception recovery. */
void generate(const char* scope)
{
  using namespace sc_core;
  std::vector<std::string> values{sc_gen_unique_name("scope_item"), sc_gen_unique_name("scope_item", false),
                                  sc_gen_unique_name("scope_first", true), sc_gen_unique_name("scope_first", true)};
  if (values != std::vector<std::string>{"scope_item_0", "scope_item_1", "scope_first", "scope_first_1"})
    throw std::runtime_error("counter");
  try
  {
    sc_gen_unique_name("");
    throw std::runtime_error("empty name accepted");
  }
  catch (const sc_report&)
  {
  }
  std::string next = sc_gen_unique_name("scope_item");
  if (next != "scope_item_2" || values[0] != "scope_item_0") throw std::runtime_error("recovery");
  std::string record = std::string("UNIQUE_NAME_REFERENCE ") + scope;
  for (auto& value : values) record += " " + value;
  records.push_back(record + " " + next);
}
/** @brief Own two processes whose counters are independent of the module. */
struct model : sc_core::sc_module
{
  SC_HAS_PROCESS(model);
  model(sc_core::sc_module_name name, bool workers) : sc_module(name)
  {
    generate(this->name());
    if (workers)
    {
      SC_THREAD(worker_a);
      SC_THREAD(worker_b);
    }
  }
  void worker_a()
  {
    generate("worker_a");
  }
  void worker_b()
  {
    generate("worker_b");
  }
};
/** @brief Assert global counter continuity outside construction/process scopes. */
int sc_main(int, char**)
{
  using namespace sc_core;
  generate("global");
  model first("first_module", true), second("second_module", false);
  if (std::string(sc_gen_unique_name("scope_item")) != "scope_item_3") return 2;
  sc_start(1, SC_NS);
  if (std::string(sc_gen_unique_name("scope_item")) != "scope_item_4") return 3;
  sc_stop();
  if (records.size() != 5) return 4;
  std::sort(records.begin(), records.end());
  for (auto& record : records) std::cout << record << "\n";
  return 0;
}
