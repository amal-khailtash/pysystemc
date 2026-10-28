/** @file sc_core_vector_container_public.cpp
 *  @brief Native reference for public sc_vector and sc_vector_base behavior.
 *  @details Each invocation owns a fresh singleton SystemC kernel. Vectors
 *  own their elements; ports borrow their fixture's channels. Creator calls
 *  are synchronous native calls and no Python callback or GIL is involved.
 */
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

#include "sc_core_vector_container_probe.h"

namespace
{

/** @brief Stop the reference as soon as one native contract differs. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Return the report identifier raised by a SystemC operation. */
template <typename Action>
int report_id(Action action)
{
  try
  {
    action();
  }
  catch (const sc_core::sc_report& report)
  {
    return report.get_id();
  }
  return -1;
}

/** @brief Join the stable integer values from vector binding paths. */
std::string join_values(const std::vector<int>& values)
{
  std::string result;
  for (std::size_t index = 0; index < values.size(); ++index)
  {
    if (index) result += ',';
    result += std::to_string(values[index]);
  }
  return result;
}

}  // namespace

/** @brief Compare public constructors, init, access, binding, and base APIs. */
int sc_main(int, char**)
{
  using namespace sc_core;
  using namespace sc_core_vector_container_probe;

  using ElementVector = sc_vector<Element>;
  ElementVector default_vector;
  default_vector.init(2);
  ElementVector named_vector("named_vector");
  named_vector.init(2);
  ElementVector sized_vector("sized_vector", 3);
  ElementVector creator_vector("creator_vector", 3, ElementCreator{});
  ElementVector init_creator_vector("init_creator_vector");
  init_creator_vector.init(3, ElementCreator{});

  verify(default_vector.size() == 2 && named_vector.size() == 2 && sized_vector.size() == 3 &&
           creator_vector.size() == 3 && init_creator_vector.size() == 3,
         "default, named, sized, and creator vector construction");
  verify(default_vector.kind() == std::string("sc_vector"), "sc_vector_base kind");
  verify(creator_vector.get_elements().size() == 3, "sc_vector_base element objects");
  verify(base_aliases_match(), "sc_vector_base public typedefs");
  verify(creator_vector[0].ordinal == 0 && creator_vector[1].ordinal == 1 && creator_vector[2].ordinal == 2 &&
           init_creator_vector[0].ordinal == 0 && init_creator_vector[1].ordinal == 1 &&
           init_creator_vector[2].ordinal == 2,
         "native creator constructor and init indices");

  const int before_native_creator_scope = destroyed_counter();
  {
    ElementVector native_creator_scope("native_creator_scope", 3, ElementCreator{});
    ElementVector native_init_scope("native_init_scope");
    native_init_scope.init(3, ElementCreator{});
    verify(native_creator_scope[2].ordinal == 2 && native_init_scope[1].ordinal == 1,
           "transient native creator values");
  }
  verify(destroyed_counter() - before_native_creator_scope == 6, "native creator vectors release all owned elements");

  creator_vector[0].ordinal = 10;
  creator_vector.at(1).ordinal = 21;
  creator_vector.begin()[2].ordinal = 32;
  const ElementVector& const_creator_vector = creator_vector;
  verify(const_creator_vector[0].ordinal == 10 && const_creator_vector.at(1).ordinal == 21,
         "const and mutable subscript and at overloads");
  verify(creator_vector.end() - creator_vector.begin() == 3 && creator_vector.cend() - creator_vector.cbegin() == 3 &&
           creator_vector.begin()[2].ordinal == 32,
         "mutable and const vector iterator access");

  Element* loose = ElementVector::create_element("loose_element", 0);
  verify(std::string(loose->name()) == "loose_element", "static create_element name");
  const int before_loose_destroy = destroyed_counter();
  destroy_element(loose);
  verify(destroyed_counter() == before_loose_destroy + 1, "standalone element ownership release");

  const int before_vector_destroy = destroyed_counter();
  {
    ElementVector lifecycle_vector("lifecycle_vector", 2, ElementCreator{});
    verify(lifecycle_vector.get_elements().size() == 2, "lifecycle vector object casts");
  }
  const int lifecycle_destroyed = destroyed_counter() - before_vector_destroy;
  verify(lifecycle_destroyed == 2, "vector destructor releases each owned element");

  int init_report = -1;
  const sc_actions old_error_actions =
    sc_report_handler::set_actions(SC_ID_VECTOR_INIT_CALLED_TWICE_, SC_ERROR, SC_THROW);
  init_report = report_id([&] { sized_vector.init(4); });
  sc_report_handler::set_actions(SC_ID_VECTOR_INIT_CALLED_TWICE_, SC_ERROR, old_error_actions);
  verify(init_report == 805 && sized_vector.size() == 3, "repeated init report and existing size");

  ElementVector empty_vector("empty_vector");
  const sc_actions old_warning_actions = sc_report_handler::set_actions(SC_ID_VECTOR_BIND_EMPTY_, SC_WARNING, SC_THROW);
  const int empty_source_report = report_id([&] { empty_vector.report_empty_bind("sc_in", true); });
  const int empty_destination_report = report_id([&] { creator_vector.report_empty_bind("sc_in", false); });
  sc_report_handler::set_actions(SC_ID_VECTOR_BIND_EMPTY_, SC_WARNING, old_warning_actions);
  verify(empty_source_report == 807 && empty_destination_report == 807, "empty binding report paths");

  IntegerSignalVector signals("python_native_signals", 3, IntegerSignalCreator{});
  verify(signals.size() == 3, "arbitrary sc_signal<int> template vector");

  BindFixture fixture("fixture");
  fixture.bind_all();
  sc_start(SC_ZERO_TIME);
  const std::vector<int> bound = binding_values(fixture);
  verify(bound == std::vector<int>{2, 1, 2, 6, 4, 1, 5, 5, 5, 2, 4}, "container, iterator, call, and assembly binds");
  verify(signals[0].read() == 0 && signals.at(1).read() == 7 && signals[2].read() == 14,
         "custom signal creator values");

  std::cout << "SC_CORE_VECTOR_CONTAINER sizes=2,2,3,3,3 kind=" << default_vector.kind()
            << " elements=" << creator_vector.get_elements().size() << " creator=10,21,32"
            << " const=10,21 iter=3,3 signal=0,7,14 binds=" << join_values(bound) << " reports=" << init_report << ','
            << empty_source_report << ',' << empty_destination_report
            << " lifecycle=1,2 aliases=" << base_aliases_match() << '\n';
  return 0;
}
