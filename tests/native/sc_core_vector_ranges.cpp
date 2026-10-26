/** @file sc_core_vector_ranges.cpp
 * @brief Native reference for sc_vector assembly bind and call range contracts.
 * @details The executable owns a fresh SystemC kernel. Each scenario owns its
 * destination modules and source signals until simulation finishes. Binding is
 * synchronous native work without Python callbacks or GIL interaction.
 */
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <systemc>
#include <unordered_map>
#include <vector>

#include "sc_core_vector_assembly_probe.h"

namespace
{

using namespace sc_core;
using namespace sc_core_vector_assembly_probe;

/** @brief Keep all objects used by one range contract alive through simulation. */
struct RangeCase
{
  std::string name;
  std::unique_ptr<ElementVector> elements;
  std::unique_ptr<SignalVector> sources;
  std::unique_ptr<SignalVector> fallback;
  std::unordered_map<std::size_t, std::size_t> expected_sources;
  std::size_t return_offset = 0;
  std::size_t bound_count = 0;
};

/** @brief Stop the native reference when a checked SystemC contract differs. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Apply one bind or operator() range and verify identity and return position.
 * @param name Unique prefix for SystemC objects owned by the case.
 * @param destination_size Number of destination input ports.
 * @param source_size Number of source signals.
 * @param destination_start Destination assembly index where binding begins.
 * @param source_start Inclusive source iterator index.
 * @param source_stop Exclusive source iterator index.
 * @param use_call Select operator() when true and bind when false.
 * @return Scenario owners and the independently computed native results.
 * @details The case owns vectors and their elements. Ports borrow signals; every
 * unmatched port is checked unbound before it is connected to its fallback
 * signal. No callback, exception translation, or GIL state is involved.
 */
std::unique_ptr<RangeCase> exercise_case(const std::string& name, std::size_t destination_size, std::size_t source_size,
                                         std::size_t destination_start, std::size_t source_start,
                                         std::size_t source_stop, bool use_call)
{
  auto result = std::make_unique<RangeCase>();
  result->name = name;
  result->elements = std::make_unique<ElementVector>((name + "_elements").c_str(), destination_size);
  result->sources = std::make_unique<SignalVector>((name + "_sources").c_str(), source_size);
  result->fallback = std::make_unique<SignalVector>((name + "_fallback").c_str(), destination_size);
  for (std::size_t index = 0; index < source_size; ++index)
    (*result->sources)[index].write(((index + name.size()) % 2) != 0);
  for (std::size_t index = 0; index < destination_size; ++index) (*result->fallback)[index].write(false);

  auto assembly = result->elements->assemble(&Element::input);
  auto source_first = result->sources->begin() + source_start;
  auto source_last = result->sources->begin() + source_stop;
  auto destination_first = assembly.begin() + destination_start;
  auto returned = use_call ? assembly(source_first, source_last, destination_first)
                           : assembly.bind(source_first, source_last, destination_first);
  result->bound_count = std::min(source_stop - source_start, destination_size - destination_start);
  result->return_offset = static_cast<std::size_t>(returned - assembly.begin());
  verify(result->return_offset == destination_start + result->bound_count, "returned destination iterator");

  for (std::size_t offset = 0; offset < result->bound_count; ++offset)
    result->expected_sources.emplace(destination_start + offset, source_start + offset);
  for (std::size_t index = 0; index < destination_size; ++index)
  {
    sc_in<bool>& port = (*result->elements)[index].input;
    (*result->elements)[index].spare((*result->fallback)[index]);
    const auto found = result->expected_sources.find(index);
    if (found != result->expected_sources.end())
      verify(port.get_interface() == &(*result->sources)[found->second], "bound source identity");
    else
    {
      verify(port.get_interface() == nullptr, "unmatched destination left unbound");
      port((*result->fallback)[index]);
    }
  }
  return result;
}

}  // namespace

/** @brief Run native range contract scenarios and emit their parity record. */
int sc_main(int, char**)
{
  std::vector<std::unique_ptr<RangeCase>> cases;
  cases.push_back(exercise_case("bind_short_source", 4, 2, 0, 0, 2, false));
  cases.push_back(exercise_case("call_short_destination", 2, 4, 0, 0, 4, true));
  cases.push_back(exercise_case("bind_subrange", 5, 5, 1, 1, 4, false));
  cases.push_back(exercise_case("call_subrange", 4, 5, 2, 1, 5, true));

  sc_start(1, SC_NS);
  verify(sc_time_stamp() == sc_time(1, SC_NS), "simulation time");
  for (const auto& current : cases)
  {
    for (std::size_t source_index = 0; source_index < current->sources->size(); ++source_index)
      verify((*current->sources)[source_index].read() == ((source_index + current->name.size()) % 2 != 0),
             "source signal retained its stimulus");
    for (std::size_t index = 0; index < current->elements->size(); ++index)
    {
      const auto found = current->expected_sources.find(index);
      const bool expected =
        found != current->expected_sources.end() ? ((found->second + current->name.size()) % 2 != 0) : false;
      verify(found != current->expected_sources.end() || !(*current->fallback)[index].read(),
             "fallback signal remains false");
      verify(sample_count(*current->elements, index) == 1, "bound module callback count");
      verify(last_sample(*current->elements, index) == expected, "bound module sampled expected value");
      verify((*current->elements)[index].input.read() == expected, "bound input reads expected value");
    }
  }
  sc_stop();

  std::cout << "SC_CORE_VECTOR_RANGES returns=";
  for (std::size_t index = 0; index < cases.size(); ++index)
    std::cout << (index ? "," : "") << cases[index]->return_offset;
  std::cout << " bound=";
  for (std::size_t index = 0; index < cases.size(); ++index)
    std::cout << (index ? "," : "") << cases[index]->bound_count;
  std::cout << " time=1ns\n";
  return 0;
}
