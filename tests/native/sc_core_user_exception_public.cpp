/** @file sc_core_user_exception_public.cpp
 * @brief Native reference for custom user exceptions and descendant injection.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <functional>
#include <iostream>
#include <string>
#include <systemc>
#include <vector>

#include "sc_core_user_exception_probe.h"

namespace
{
std::vector<std::string> records;
std::vector<sc_core::sc_process_handle> handles;

/** @brief Record an ordered callback transition in resolution ticks. */
void stamp(const std::string& label)
{
  records.push_back(label + " " + std::to_string(sc_core::sc_time_stamp().value()));
}
/** @brief Record cleanup for return or exception propagation. */
struct Cleanup
{
  std::string label;
  ~Cleanup()
  {
    stamp("cleanup " + label);
  }
};
/** @brief Catch injected values and continue waiting with native thread semantics. */
template <class Exception>
void worker(const std::string& label)
{
  using namespace sc_core;
  stamp("enter " + label);
  if (label == "parent")
    handles.push_back(sc_spawn(std::function<void()>([] { worker<Exception>("child"); }), "exception_child"));
  Cleanup cleanup{label};
  try
  {
    wait(5, SC_NS);
  }
  catch (const Exception& error)
  {
    if (std::string(error.what()) != "native user exception") throw;
    stamp("caught " + label);
    wait(1, SC_NS);
  }
  stamp("done " + label);
}
/** @brief Inject one copyable exception with the selected descendant policy. */
template <class Exception>
int run(const Exception& exception, bool include_children, const char* kind)
{
  using namespace sc_core;
  auto parent = sc_spawn(std::function<void()>([] { worker<Exception>("parent"); }), "exception_parent");
  handles.push_back(parent);
  sc_spawn(std::function<void()>(
             [&]
             {
               wait(1, SC_NS);
               if (include_children)
                 parent.throw_it(exception, SC_INCLUDE_DESCENDANTS);
               else
                 parent.throw_it(exception);
               stamp("controller");
             }),
           "exception_controller");
  sc_start(6, SC_NS);
  for (const auto& handle : handles)
    if (!handle.terminated()) return 2;
  for (const auto& record : records) std::cout << "USER_EXCEPTION " << record << '\n';
  std::cout << "USER_EXCEPTION_OK " << kind << ' ' << include_children << '\n';
  sc_stop();
  return 0;
}
}  // namespace

/** @brief Compare standard and custom exception types in fresh kernel runs. */
int sc_main(int argc, char** argv)
{
  bool descendants = argc > 1 && std::string(argv[1]) == "descendants";
  bool standard = argc > 2 && std::string(argv[2]) == "runtime";
  if (standard) return run(std::runtime_error("native user exception"), descendants, "runtime");
  return run(UserProcessException(41), descendants, "custom");
}
