/** @file sc_core_resolved_scalar_output_public.cpp
 * @brief Native reference for resolved scalar inout and output ports.
 * @details Checks every public constructor, alias and assignment signature,
 *          lifecycle hook, initialization path, resolved multiwriter value,
 *          diagnostic and VCD transition without Python callbacks.
 */
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>
#include <vector>

#include "sc_core_resolved_scalar_output_probe.h"

using namespace sc_core;
using namespace sc_dt;
using namespace pysystemc_resolved_scalar_output;

namespace
{
using logic_type = sc_logic;

/** @brief Concrete module exposing SystemC's protected module constructor. */
struct Scope : sc_module
{
  explicit Scope(sc_module_name name) : sc_module(name)
  {
  }
};

std::vector<int> report_ids;

/** @brief Copy a public report ID while its native diagnostic is live. */
void capture_report(const sc_report& report, const sc_actions&)
{
  report_ids.push_back(report.get_id());
}

/** @brief Retain each channel borrowed by constructor and assignment tests. */
using Channels = std::vector<std::unique_ptr<sc_signal_resolved>>;

/** @brief Retain each concrete port and its owner through elaboration. */
template <class Port>
using Ports = std::vector<std::unique_ptr<Port>>;

/** @brief Retain ordinary inout parent ports for constructor coverage. */
using ParentPorts = std::vector<std::unique_ptr<sc_inout<logic_type>>>;

/** @brief Exercise all eight resolved scalar port constructors.
 * @tparam Port The sc_inout_resolved or sc_out_resolved concrete type.
 * @param prefix Unique native object-name prefix.
 * @param channels Retains channels borrowed by ports and parents.
 * @param ports Owns target and concrete parent ports.
 * @param parents Owns ordinary inout parent ports.
 * @return The eight target ports, in declaration order.
 */
template <class Port>
std::vector<Port*> make_constructor_matrix(const std::string& prefix, Channels& channels, Ports<Port>& ports,
                                           ParentPorts& parents)
{
  std::vector<Port*> targets;
  const auto channel = [&](int index) -> sc_signal_resolved&
  {
    const std::string name = prefix + "_channel_" + std::to_string(index);
    channels.emplace_back(new sc_signal_resolved(name.c_str()));
    return *channels.back();
  };
  const auto retain = [&](Port* port) -> Port*
  {
    ports.emplace_back(port);
    targets.push_back(port);
    return port;
  };
  const auto hold = [&](Port* port) -> Port*
  {
    ports.emplace_back(port);
    return port;
  };

  Port* port = retain(new Port());
  port->bind(channel(0));
  port = retain(new Port((prefix + "_named").c_str()));
  port->bind(channel(1));
  retain(new Port(channel(2)));
  retain(new Port((prefix + "_named_interface").c_str(), channel(3)));

  parents.emplace_back(new sc_inout<logic_type>((prefix + "_parent_4").c_str()));
  auto* parent = parents.back().get();
  parent->bind(channel(4));
  retain(new Port(*parent));
  parents.emplace_back(new sc_inout<logic_type>((prefix + "_parent_5").c_str()));
  parent = parents.back().get();
  parent->bind(channel(5));
  retain(new Port((prefix + "_named_parent_port").c_str(), *parent));

  port = hold(new Port((prefix + "_concrete_parent_6").c_str(), channel(6)));
  retain(new Port(*port));
  port = hold(new Port((prefix + "_concrete_parent_7").c_str(), channel(7)));
  retain(new Port((prefix + "_named_concrete_parent").c_str(), *port));

  if (targets.size() != 8) throw std::runtime_error("resolved scalar output constructor count");
  for (Port* target : targets)
  {
    if (target->bind_count() != 1 || std::string(target->kind()).empty())
    {
      throw std::runtime_error("resolved scalar output constructor binding or kind");
    }
  }
  return targets;
}

/** @brief Explicitly dispatch each native assignment declaration by type.
 * @details These helpers encode public alias references in their parameters so
 *          overload resolution is checked against the documented signature.
 */
template <class Port>
bool assign_data(Port& destination, const typename Port::data_type& value)
{
  return &destination.operator=(value) == &destination;
}

template <class Port>
bool assign_interface(Port& destination, const typename Port::in_if_type& value)
{
  return &destination.operator=(value) == &destination;
}

template <class Port>
bool assign_input_port(Port& destination, const typename Port::in_port_type& value)
{
  return &destination.operator=(value) == &destination;
}

template <class Port>
bool assign_inout_port(Port& destination, const typename Port::inout_port_type& value)
{
  return &destination.operator=(value) == &destination;
}

template <class Port>
bool assign_self(Port& destination, const typename Port::this_type& value)
{
  return &destination.operator=(value) == &destination;
}

/** @brief Check aliases against native types at compile time. */
template <class Port, class Base>
void check_aliases()
{
  static_assert(std::is_same<typename Port::this_type, Port>::value, "this_type");
  static_assert(std::is_same<typename Port::data_type, logic_type>::value, "data_type");
  static_assert(std::is_same<typename Port::in_if_type, typename Base::in_if_type>::value, "in_if_type");
  static_assert(std::is_same<typename Port::in_port_type, typename Base::in_port_type>::value, "in_port_type");
  static_assert(std::is_same<typename Port::inout_if_type, typename Base::inout_if_type>::value, "inout_if_type");
  static_assert(std::is_same<typename Port::inout_port_type, typename Base::inout_port_type>::value, "inout_port_type");
}

/** @brief Resolve two independent output writers and record scalar samples. */
struct Observer : sc_module
{
  sc_in_resolved input{"input"};
  std::vector<std::pair<std::string, std::string>> values;

  SC_HAS_PROCESS(Observer);

  /** @brief Bind the resolved input and register a noninitializing method. */
  Observer(sc_module_name name, sc_signal_resolved& channel) : sc_module(name)
  {
    input.bind(channel);
    SC_METHOD(on_change);
    sensitive << input;
    dont_initialize();
  }

  /** @brief Save the resolved value and native time for each transition. */
  void on_change()
  {
    std::ostringstream value;
    value << input->read();
    std::ostringstream time;
    time << sc_time_stamp();
    values.emplace_back(time.str(), value.str());
  }
};

/** @brief Drive one output port, then release its resolved writer. */
template <class Port>
struct Driver : sc_module
{
  Port output{"output"};
  const sc_logic value;
  bool initialized = false;

  SC_HAS_PROCESS(Driver);

  /** @brief Bind the concrete output port and register its writer method. */
  Driver(sc_module_name name, sc_signal_resolved& channel, sc_logic value_) : sc_module(name), value(value_)
  {
    output.bind(channel);
    SC_METHOD(drive);
  }

  /** @brief Write one conflicting logic value and release it after one ns. */
  void drive()
  {
    output.write(initialized ? SC_LOGIC_Z : value);
    if (!initialized)
    {
      initialized = true;
      next_trigger(1, SC_NS);
    }
  }
};

/** @brief Run the two resolved-port type diagnostics in an independent run. */
int run_errors()
{
  sc_signal<logic_type> ordinary_inout("ordinary_inout_logic");
  sc_signal<logic_type> ordinary_output("ordinary_output_logic");
  std::unique_ptr<Scope> module;
  std::unique_ptr<sc_inout_resolved> inout_port;
  std::unique_ptr<sc_out_resolved> output_port;
  {
    sc_module_name name("resolved_scalar_output_errors");
    module.reset(new Scope(name));
    inout_port.reset(new sc_inout_resolved("bad_inout"));
    output_port.reset(new sc_out_resolved("bad_output"));
    inout_port->bind(ordinary_inout);
    output_port->bind(ordinary_output);
  }
  const sc_report_handler_proc old_handler = sc_report_handler::set_handler(&capture_report);
  const sc_actions old_actions = sc_report_handler::set_actions(SC_ERROR, SC_DO_NOTHING);
  sc_start(SC_ZERO_TIME);
  sc_report_handler::set_actions(SC_ERROR, old_actions);
  sc_report_handler::set_handler(old_handler);
  if (report_ids != std::vector<int>{117, 117}) throw std::runtime_error("resolved scalar output diagnostics");
  std::cout << "RESOLVED_SCALAR_OUTPUT_ERRORS 117|117\n";
  return 0;
}

}  // namespace

/** @brief Verify constructors, assignments, lifecycle, resolution and VCD. */
int sc_main(int argc, char** argv)
{
  check_aliases<sc_inout_resolved, sc_inout<logic_type>>();
  check_aliases<sc_out_resolved, sc_inout_resolved>();
  if (argc > 1 && std::string(argv[1]) == "errors") return run_errors();

  reset_counters();
  std::unique_ptr<Scope> root;
  std::unique_ptr<sc_module_name> root_name(new sc_module_name("resolved_scalar_output_top"));
  root.reset(new Scope(*root_name));
  if (!verify_virtual_destruction()) return 10;
  const int destruction_inout = destruction_count(0);
  const int destruction_output = destruction_count(1);

  Channels inout_channels;
  Channels output_channels;
  Ports<sc_inout_resolved> inout_ports;
  Ports<sc_out_resolved> output_ports;
  ParentPorts inout_parents;
  ParentPorts output_parents;
  const auto inout_matrix =
    make_constructor_matrix<sc_inout_resolved>("sc_inout_resolved", inout_channels, inout_ports, inout_parents);
  const auto output_matrix =
    make_constructor_matrix<sc_out_resolved>("sc_out_resolved", output_channels, output_ports, output_parents);

  Channels assignment_channels;
  Ports<sc_inout_resolved> inout_assignments;
  Ports<sc_out_resolved> output_assignments;
  sc_signal_resolved inout_source("inout_assignment_source", SC_LOGIC_0);
  sc_signal_resolved output_source("output_assignment_source", SC_LOGIC_0);
  sc_in<logic_type> inout_input_source("inout_input_source");
  sc_inout<logic_type> inout_parent_source("inout_parent_source");
  sc_in<logic_type> output_input_source("output_input_source");
  sc_inout<logic_type> output_parent_source("output_parent_source");
  sc_inout_resolved inout_self_source("inout_self_source", inout_source);
  sc_out_resolved output_self_source("output_self_source", output_source);
  inout_input_source.bind(inout_source);
  inout_parent_source.bind(inout_source);
  output_input_source.bind(output_source);
  output_parent_source.bind(output_source);
  std::vector<std::string> assignment_values;
  const auto add_inout_assignment = [&](const std::string& suffix, auto call)
  {
    assignment_channels.emplace_back(new sc_signal_resolved(("inout_assign_" + suffix).c_str()));
    inout_assignments.emplace_back(new sc_inout_resolved(("inout_assign_port_" + suffix).c_str()));
    inout_assignments.back()->bind(*assignment_channels.back());
    if (!call(*inout_assignments.back())) throw std::runtime_error("inout assignment return reference");
    assignment_values.push_back("inout:" + suffix);
  };
  const auto add_output_assignment = [&](const std::string& suffix, auto call)
  {
    assignment_channels.emplace_back(new sc_signal_resolved(("out_assign_" + suffix).c_str()));
    output_assignments.emplace_back(new sc_out_resolved(("out_assign_port_" + suffix).c_str()));
    output_assignments.back()->bind(*assignment_channels.back());
    if (!call(*output_assignments.back())) throw std::runtime_error("output assignment return reference");
    assignment_values.push_back("output:" + suffix);
  };

  add_inout_assignment("data", [&](auto& port) { return assign_data(port, logic_type(SC_LOGIC_1)); });
  add_inout_assignment("interface", [&](auto& port) { return assign_interface(port, inout_source); });
  add_inout_assignment("input", [&](auto& port) { return assign_input_port(port, inout_input_source); });
  add_inout_assignment("inout", [&](auto& port) { return assign_inout_port(port, inout_parent_source); });
  add_inout_assignment("self", [&](auto& port) { return assign_self(port, inout_self_source); });
  add_output_assignment("data", [&](auto& port) { return assign_data(port, logic_type(SC_LOGIC_1)); });
  add_output_assignment("interface", [&](auto& port) { return assign_interface(port, output_source); });
  add_output_assignment("input", [&](auto& port) { return assign_input_port(port, output_input_source); });
  add_output_assignment("inout", [&](auto& port) { return assign_inout_port(port, output_parent_source); });
  add_output_assignment("self", [&](auto& port) { return assign_self(port, output_self_source); });

  sc_signal_resolved initialized_data("initialized_data");
  sc_signal_resolved initialize_source("initialize_source", SC_LOGIC_0);
  sc_signal_resolved initialized_interface("initialized_interface");
  sc_inout_resolved init_data_port("init_data_port", initialized_data);
  sc_out_resolved init_interface_port("init_interface_port", initialized_interface);
  init_data_port.initialize(logic_type(SC_LOGIC_1));
  init_interface_port.initialize(initialize_source);

  sc_signal_resolved resolved("resolved_multiwriter", SC_LOGIC_Z);
  Observer observer("resolved_output_observer", resolved);
  Driver<sc_out_resolved> writer_zero("resolved_output_writer_zero", resolved, SC_LOGIC_0);
  Driver<sc_inout_resolved> writer_one("resolved_output_writer_one", resolved, SC_LOGIC_1);

  InoutResolvedProbe hook_inout("hook_inout_resolved");
  sc_signal_resolved hook_inout_channel("hook_inout_channel");
  hook_inout.bind(hook_inout_channel);
  OutResolvedProbe hook_output("hook_out_resolved");
  sc_signal_resolved hook_output_channel("hook_output_channel");
  hook_output.bind(hook_output_channel);

  sc_trace_file* trace = sc_create_vcd_trace_file("native_resolved_scalar_output_public");
  trace->set_time_unit(1, SC_NS);
  sc_trace(trace, resolved, "resolved");
  sc_trace(trace, initialized_data, "initialized_data");
  sc_trace(trace, initialized_interface, "initialized_interface");
  root_name.reset();
  sc_start(2, SC_NS);
  sc_close_vcd_trace_file(trace);

  if (inout_matrix.size() != 8 || output_matrix.size() != 8) return 11;
  if (elaboration_count(0) != 1 || elaboration_count(1) != 1) return 12;
  if (destruction_inout != 1 || destruction_output != 1) return 13;
  if (init_data_port.read() != SC_LOGIC_1 || init_interface_port.read() != SC_LOGIC_0) return 14;
  const std::vector<std::pair<std::string, std::string>> expected{{"0 s", "X"}, {"1 ns", "Z"}};
  if (observer.values != expected) return 15;
  for (std::size_t index = 0; index < inout_assignments.size(); ++index)
  {
    const sc_logic expected_value = index == 0 || index == 5 ? SC_LOGIC_1 : SC_LOGIC_0;
    if (inout_assignments[index]->read() != expected_value) return 16;
    if (output_assignments[index]->read() != expected_value) return 17;
  }

  std::cout << "RESOLVED_SCALAR_OUTPUT_CTORS " << inout_matrix.size() << '|' << output_matrix.size() << '\n';
  std::cout << "RESOLVED_SCALAR_OUTPUT_ALIASES 7|7\n";
  std::cout << "RESOLVED_SCALAR_OUTPUT_ASSIGNMENTS 5|5\n";
  std::cout << "RESOLVED_SCALAR_OUTPUT_INITIALIZE " << init_data_port.read() << '|' << init_interface_port.read()
            << '\n';
  std::cout << "RESOLVED_SCALAR_OUTPUT_HOOKS " << elaboration_count(0) << '|' << elaboration_count(1) << '\n';
  std::cout << "RESOLVED_SCALAR_OUTPUT_DESTRUCTION " << destruction_inout << '|' << destruction_output << '\n';
  std::cout << "RESOLVED_SCALAR_OUTPUT_SAMPLE ";
  for (std::size_t index = 0; index < observer.values.size(); ++index)
  {
    if (index) std::cout << ',';
    std::cout << observer.values[index].first << ':' << observer.values[index].second;
  }
  std::cout << '\n';
  std::cout << "RESOLVED_SCALAR_OUTPUT_ASSIGN_VALUES ";
  for (std::size_t index = 0; index < inout_assignments.size(); ++index)
  {
    if (index) std::cout << ',';
    std::cout << inout_assignments[index]->read();
  }
  std::cout << '|';
  for (std::size_t index = 0; index < output_assignments.size(); ++index)
  {
    if (index) std::cout << ',';
    std::cout << output_assignments[index]->read();
  }
  std::cout << '\n';
  std::cout << "RESOLVED_SCALAR_OUTPUTS_OK\n";

  inout_ports.clear();
  output_ports.clear();
  inout_assignments.clear();
  output_assignments.clear();
  inout_parents.clear();
  output_parents.clear();
  sc_stop();
  return 0;
}
