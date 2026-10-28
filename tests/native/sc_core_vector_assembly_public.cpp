/** @file sc_core_vector_assembly_public.cpp
 * @brief Native reference for public sc_vector member-assembly behavior.
 * @details Each run owns one fresh SystemC kernel. Vectors own their modules
 * and channels; assemblies borrow vectors and member pointers. Binding calls
 * are synchronous native work without Python callbacks or GIL interaction.
 */
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

#include "sc_core_vector_assembly_probe.h"

namespace
{

/** @brief Stop the reference immediately when a native contract differs. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Return the identifier of a SystemC report raised by one operation. */
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

}  // namespace

/** @brief Compare all public assembly overload families with their Python ports. */
int sc_main(int, char**)
{
  using namespace sc_core;
  using namespace sc_core_vector_assembly_probe;

  verify(aliases_match_native_types(), "assembly typedef aliases");
  int empty_assembly_report = -1;
  {
    ElementVector empty_elements("empty_elements");
    InputAssembly empty_assembly = input_assembly(empty_elements);
    const sc_actions old_warning_actions =
      sc_report_handler::set_actions(SC_ID_VECTOR_BIND_EMPTY_, SC_WARNING, SC_THROW);
    empty_assembly_report = report_id([&] { empty_assembly.report_empty_bind("sc_in", true); });
    sc_report_handler::set_actions(SC_ID_VECTOR_BIND_EMPTY_, SC_WARNING, old_warning_actions);
    verify(empty_assembly_report == 807, "assembly empty-bind report");
  }

  std::array<std::unique_ptr<ElementVector>, 10> elements;
  std::array<std::unique_ptr<SignalVector>, 10> signals;
  std::array<std::unique_ptr<SignalVector>, 10> spare_signals;
  std::array<std::unique_ptr<ChannelVector>, 10> channels;
  SignalVector empty_signals("empty_signals");
  std::vector<InputAssembly> assemblies;
  assemblies.reserve(10);
  int empty_source_report = -1;
  int empty_destination_report = -1;
  for (std::size_t group = 0; group < elements.size(); ++group)
  {
    elements[group] = std::make_unique<ElementVector>(("elements_" + std::to_string(group)).c_str(), 2);
    signals[group] = std::make_unique<SignalVector>(("signals_" + std::to_string(group)).c_str(), 2);
    spare_signals[group] = std::make_unique<SignalVector>(("spare_signals_" + std::to_string(group)).c_str(), 2);
    channels[group] = std::make_unique<ChannelVector>(("channels_" + std::to_string(group)).c_str(), 2);
    assemblies.push_back(input_assembly(*elements[group]));
    SpareAssembly spare = spare_assembly(*elements[group]);
    spare.bind(*spare_signals[group]);
    for (std::size_t index = 0; index < 2; ++index)
    {
      const bool value = ((group + index) % 2) != 0;
      (*signals[group])[index].write(value);
      (*spare_signals[group])[index].write(false);
      (*channels[group])[index].value.write(value);
    }
  }
  for (std::size_t group = 0; group < assemblies.size(); ++group)
  {
    InputAssembly& assembly = assemblies[group];
    ElementVector& vector = *elements[group];
    SignalVector& signal_vector = *signals[group];
    ChannelAssembly channel_members = channel_assembly(*channels[group]);
    verify(assembly.size() == 2 && std::string(assembly.kind()) == "sc_vector_assembly" &&
             std::string(assembly.name()) == vector.name(),
           "assembly basic metadata");
    verify(input_identity(assembly, 0, vector) && input_identity(assembly, 1, vector), "member index identity");
    verify(input_elements_identity(assembly, vector), "assembly get_elements identity");
    verify(channel_elements_identity(channel_members, *channels[group]), "channel assembly identity");
    verify(assembly.get_elements().size() == 2 && channel_members.get_elements().size() == 2,
           "assembly object elements");

    if (group == 0)
    {
      InputAssembly copied(assembly);
      verify(input_identity(copied, 0, vector), "assembly copy constructor");
      InputAssembly assigned(assembly);
      SpareAssembly spare = spare_assembly(vector);
      assigned = spare;
      verify(spare_identity(assigned, 0, vector), "assembly copy assignment");
      InputAssembly alternate = spare_assembly(vector);
      copied.swap(alternate);
      verify(spare_identity(copied, 0, vector), "assembly swap");
      copied.swap(alternate);
      verify(input_identity(copied, 0, vector), "assembly swap restoration");

      const sc_actions old_warning_actions =
        sc_report_handler::set_actions(SC_ID_VECTOR_BIND_EMPTY_, SC_WARNING, SC_THROW);
      empty_source_report = report_id([&] { spare.bind(empty_signals.begin(), empty_signals.end()); });
      empty_destination_report =
        report_id([&] { spare.bind(signal_vector.begin(), signal_vector.end(), vector.end()); });
      sc_report_handler::set_actions(SC_ID_VECTOR_BIND_EMPTY_, SC_WARNING, old_warning_actions);
      verify(empty_source_report == 807 && empty_destination_report == 807, "empty assembly bind ranges");

      const InputAssembly& const_assembly = assembly;
      verify(input_identity(const_assembly, 0, vector) && input_identity(const_assembly, 1, vector),
             "const index and at overloads");
      verify(&assembly.begin()[0] == &assembly[0] && &assembly.end()[-1] == &assembly[1], "mutable iterator access");
      verify(&const_assembly.begin()[0] == &assembly[0] && &const_assembly.end()[-1] == &assembly[1],
             "const iterator access");
      verify(&const_assembly.cbegin()[0] == &assembly[0] && &const_assembly.cend()[-1] == &assembly[1],
             "explicit const iterator access");
      verify(assembly.end() - assembly.begin() == 2 && const_assembly.end() - const_assembly.begin() == 2 &&
               const_assembly.cend() - const_assembly.cbegin() == 2,
             "assembly iterator ranges");
    }
    else if (group == 1)
    {
      InputAssembly cross_copy(assemblies[0]);
      verify(input_identity(cross_copy, 0, *elements[0]) && input_elements_identity(cross_copy, *elements[0]),
             "cross-vector assembly copy");

      InputAssembly cross_assigned(assembly);
      cross_assigned = assemblies[0];
      verify(input_identity(cross_assigned, 1, *elements[0]) && input_elements_identity(cross_assigned, *elements[0]),
             "cross-vector assembly assignment");
      cross_assigned = cross_assigned;
      verify(input_identity(cross_assigned, 0, *elements[0]) && input_elements_identity(cross_assigned, *elements[0]),
             "cross-vector assembly self-assignment");

      InputAssembly swapped_left(assemblies[0]);
      InputAssembly swapped_right(assembly);
      swapped_left.swap(swapped_right);
      verify(input_identity(swapped_left, 0, vector) && input_elements_identity(swapped_left, vector) &&
               input_identity(swapped_right, 0, *elements[0]) && input_elements_identity(swapped_right, *elements[0]),
             "cross-vector assembly swap");
    }

    switch (group)
    {
    case 0:
      assembly.bind(signal_vector);
      break;
    case 1:
      assembly.bind(signal_vector.begin(), signal_vector.end());
      break;
    case 2:
      assembly.bind(signal_vector.begin(), signal_vector.end(), assembly.begin());
      break;
    case 3:
      assembly.bind(signal_vector.begin(), signal_vector.end(), vector.begin());
      break;
    case 4:
      assembly.bind(channel_members);
      break;
    case 5:
      assembly(signal_vector);
      break;
    case 6:
      assembly(signal_vector.begin(), signal_vector.end());
      break;
    case 7:
      assembly(signal_vector.begin(), signal_vector.end(), assembly.begin());
      break;
    case 8:
      assembly(signal_vector.begin(), signal_vector.end(), vector.begin());
      break;
    case 9:
      assembly(channel_members);
      break;
    default:
      throw std::runtime_error("unhandled assembly operation");
    }

    ObservedAssembly observed = observed_assembly(vector);
    verify(observed.size() == 2 && observed_identity(observed, 0, vector) && observed_identity(observed, 1, vector),
           "module channel member assembly");
    verify(observed.get_elements().size() == 2, "module channel get_elements");
  }

  sc_start(1, SC_NS);
  verify(sc_time_stamp() == sc_time(1, SC_NS), "assembly simulation time");
  for (std::size_t group = 0; group < elements.size(); ++group)
  {
    for (std::size_t index = 0; index < 2; ++index)
    {
      const bool expected = ((group + index) % 2) != 0;
      verify(sample_count(*elements[group], index) == 1 && last_sample(*elements[group], index) == expected,
             "bound module port sampled expected value");
      verify((*elements[group])[index].observed.read() == expected, "assembled channel value");
    }
  }
  sc_stop();

  // The vectors and their element modules are released together after simulation.
  assemblies.clear();
  elements = {};
  signals = {};
  spare_signals = {};
  channels = {};
  verify(Element::destroyed == 20, "vector module lifecycle");
  std::cout << "SC_CORE_VECTOR_ASSEMBLY count=10x2 aliases=1 bindings=10 time=1ns empty_report="
            << empty_assembly_report << ',' << empty_source_report << ',' << empty_destination_report
            << " destroyed=" << Element::destroyed << '\n';
  return 0;
}
