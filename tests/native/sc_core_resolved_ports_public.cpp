/** @file sc_core_resolved_ports_public.cpp
 * @brief Native reference for resolved scalar and vector input ports.
 * @details Verifies all input-port constructors, native aliases, resolved
 *          type diagnostics, lifecycle hooks, multi-process resolution,
 *          sampled values and VCD transitions without Python callbacks.
 */
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>
#include <vector>

#include "sc_core_resolved_ports_probe.h"

using namespace sc_core;
using namespace sc_dt;
using namespace pysystemc_resolved_ports;

namespace
{
constexpr int width = 137;
using vector_type = sc_lv<width>;

/** @brief Concrete module exposing SystemC's protected module constructor. */
struct Scope : sc_module
{
  explicit Scope(sc_module_name name) : sc_module(name)
  {
  }
};

std::vector<int> report_ids;

/** @brief Copy the public report ID while native diagnostic storage is live. */
void capture_report(const sc_report& report, const sc_actions&)
{
  report_ids.push_back(report.get_id());
}

/** @brief Destroy concrete port owners in reverse construction order. */
/** @brief Format native kernel time with SystemC's stable stream spelling. */
std::string time_text()
{
  std::ostringstream output;
  output << sc_time_stamp();
  return output.str();
}

/** @brief Retain and validate every scalar or vector input constructor form.
 * @tparam Port Resolved scalar or arbitrary-width input-port type.
 * @tparam Value Scalar logic or resolved vector value type.
 * @tparam Signal Matching resolved signal channel type.
 * @tparam InputPort Ordinary native input parent port type.
 * @tparam InoutPort Ordinary native inout parent port type.
 * @param prefix Unique native object-name prefix.
 * @param channels Retains channels borrowed by ports and their parents.
 * @param ports Owns concrete ports and concrete parent ports.
 * @param input_parents Owns ordinary input parent ports.
 * @param inout_parents Owns ordinary inout parent ports.
 * @return The ten target ports, in declaration order.
 */
template <class Port, class Value, class Signal, class InputPort, class InoutPort>
std::vector<Port*> make_input_matrix(const std::string& prefix, std::vector<std::unique_ptr<Signal>>& channels,
                                     std::vector<std::unique_ptr<Port>>& ports,
                                     std::vector<std::unique_ptr<InputPort>>& input_parents,
                                     std::vector<std::unique_ptr<InoutPort>>& inout_parents)
{
  std::vector<Port*> targets;
  const auto channel = [&](int index) -> Signal&
  {
    const std::string name = prefix + "_matrix_" + std::to_string(index);
    channels.emplace_back(new Signal(name.c_str()));
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

  input_parents.emplace_back(new InputPort((prefix + "_input_parent_4").c_str()));
  auto* input_parent = input_parents.back().get();
  input_parent->bind(channel(4));
  retain(new Port(*input_parent));
  input_parents.emplace_back(new InputPort((prefix + "_input_parent_5").c_str()));
  input_parent = input_parents.back().get();
  input_parent->bind(channel(5));
  retain(new Port((prefix + "_named_input_parent").c_str(), *input_parent));

  inout_parents.emplace_back(new InoutPort((prefix + "_inout_parent_6").c_str()));
  auto* inout_parent = inout_parents.back().get();
  inout_parent->bind(channel(6));
  retain(new Port(*inout_parent));
  inout_parents.emplace_back(new InoutPort((prefix + "_inout_parent_7").c_str()));
  inout_parent = inout_parents.back().get();
  inout_parent->bind(channel(7));
  retain(new Port((prefix + "_named_inout_parent").c_str(), *inout_parent));

  port = hold(new Port((prefix + "_concrete_parent_8").c_str(), channel(8)));
  retain(new Port(*port));
  port = hold(new Port((prefix + "_concrete_parent_9").c_str(), channel(9)));
  retain(new Port((prefix + "_named_concrete_parent").c_str(), *port));

  if (targets.size() != 10) throw std::runtime_error("resolved input constructor count");
  for (Port* candidate : targets)
  {
    if (candidate->bind_count() != 1) throw std::runtime_error("resolved input bind count");
  }
  return targets;
}

/** @brief Observe resolved scalar and width-137 transitions through ports. */
struct Observer : sc_module
{
  sc_in_resolved scalar{"scalar_input"};
  sc_in_rv<width> vector{"vector_input"};
  std::vector<std::pair<std::string, std::string>> scalar_values;
  std::vector<std::pair<std::string, std::string>> vector_values;

  SC_HAS_PROCESS(Observer);

  /** @brief Bind concrete input ports and register noninitializing methods. */
  Observer(sc_module_name name, sc_signal_resolved& scalar_channel, sc_signal_rv<width>& vector_channel)
    : sc_module(name)
  {
    scalar.bind(scalar_channel);
    vector.bind(vector_channel);
    SC_METHOD(on_scalar);
    sensitive << scalar;
    dont_initialize();
    SC_METHOD(on_vector);
    sensitive << vector;
    dont_initialize();
  }

  /** @brief Save scalar value and native time on a value-change event. */
  void on_scalar()
  {
    std::ostringstream output;
    output << scalar->read();
    scalar_values.emplace_back(time_text(), output.str());
  }

  /** @brief Save all 137 vector bits and native time on an event. */
  void on_vector()
  {
    vector_values.emplace_back(time_text(), vector->read().to_string());
  }
};

/** @brief Write independent process values to resolved scalar/vector signals. */
struct Driver : sc_module
{
  sc_signal_resolved& scalar;
  sc_signal_rv<width>& vector;
  const bool bit_value;
  bool initialized = false;

  SC_HAS_PROCESS(Driver);

  /** @brief Register one process-specific scalar/vector writer. */
  Driver(sc_module_name name, sc_signal_resolved& scalar_channel, sc_signal_rv<width>& vector_channel, bool bit_value_)
    : sc_module(name), scalar(scalar_channel), vector(vector_channel), bit_value(bit_value_)
  {
    SC_METHOD(drive_initial);
  }

  /** @brief Drive a process-specific value, then release it after one ns. */
  void drive_initial()
  {
    if (initialized)
    {
      scalar.write(SC_LOGIC_Z);
      vector.write(vector_type(std::string(width, 'Z').c_str()));
      return;
    }
    scalar.write(bit_value ? SC_LOGIC_1 : SC_LOGIC_0);
    std::string bits(width - 1, '0');
    bits.push_back(bit_value ? '1' : '0');
    vector.write(vector_type(bits.c_str()));
    initialized = true;
    next_trigger(1, SC_NS);
  }
};

/** @brief Run invalid scalar and vector bindings and capture native IDs. */
int run_errors()
{
  sc_signal<sc_logic> ordinary_scalar("ordinary_scalar");
  sc_signal<vector_type> ordinary_vector("ordinary_vector");
  std::unique_ptr<Scope> module;
  std::unique_ptr<sc_in_resolved> scalar_port;
  std::unique_ptr<sc_in_rv<width>> vector_port;
  {
    sc_module_name module_name("resolved_input_errors");
    module.reset(new Scope(module_name));
    scalar_port.reset(new sc_in_resolved("bad_scalar_input"));
    vector_port.reset(new sc_in_rv<width>("bad_vector_input"));
    scalar_port->bind(ordinary_scalar);
    vector_port->bind(ordinary_vector);
  }
  const sc_report_handler_proc previous_handler = sc_report_handler::set_handler(&capture_report);
  const sc_actions previous_actions = sc_report_handler::set_actions(SC_ERROR, SC_DO_NOTHING);
  sc_start(SC_ZERO_TIME);
  sc_report_handler::set_actions(SC_ERROR, previous_actions);
  sc_report_handler::set_handler(previous_handler);
  if (report_ids != std::vector<int>{117, 117}) throw std::runtime_error("resolved input diagnostics");
  std::cout << "RESOLVED_PORT_ERRORS 117|117\n";
  return 0;
}
}  // namespace

/** @brief Verify constructors, aliases, hooks, resolution, samples and traces. */
int sc_main(int argc, char** argv)
{
  static_assert(std::is_same<sc_in_resolved::this_type, sc_in_resolved>::value, "resolved this_type");
  static_assert(std::is_same<sc_in_resolved::data_type, sc_logic>::value, "resolved data_type");
  static_assert(std::is_same<sc_in_resolved::base_type, sc_in<sc_logic>>::value, "resolved base_type");
  static_assert(std::is_same<sc_in_rv<width>::this_type, sc_in_rv<width>>::value, "vector this_type");
  static_assert(std::is_same<sc_in_rv<width>::data_type, vector_type>::value, "vector data_type");
  static_assert(std::is_same<sc_in_rv<width>::base_type, sc_in<vector_type>>::value, "vector base_type");

  reset_counters();
  if (argc > 1 && std::string(argv[1]) == "errors") return run_errors();
  sc_module_name root_name("resolved_ports_top");
  Scope root(root_name);
  if (!verify_virtual_destruction()) return 10;

  std::vector<std::unique_ptr<sc_signal_resolved>> scalar_channels;
  std::vector<std::unique_ptr<sc_signal_rv<width>>> vector_channels;
  std::vector<std::unique_ptr<sc_in_resolved>> scalar_ports;
  std::vector<std::unique_ptr<sc_in<sc_logic>>> scalar_input_parents;
  std::vector<std::unique_ptr<sc_inout<sc_logic>>> scalar_inout_parents;
  std::vector<std::unique_ptr<sc_in_rv<width>>> vector_ports;
  std::vector<std::unique_ptr<sc_in<vector_type>>> vector_input_parents;
  std::vector<std::unique_ptr<sc_inout<vector_type>>> vector_inout_parents;
  const auto scalar_matrix =
    make_input_matrix<sc_in_resolved, sc_logic, sc_signal_resolved, sc_in<sc_logic>, sc_inout<sc_logic>>(
      "sc_in_resolved", scalar_channels, scalar_ports, scalar_input_parents, scalar_inout_parents);
  const auto vector_matrix =
    make_input_matrix<sc_in_rv<width>, vector_type, sc_signal_rv<width>, sc_in<vector_type>, sc_inout<vector_type>>(
      "sc_in_rv", vector_channels, vector_ports, vector_input_parents, vector_inout_parents);

  sc_signal_resolved scalar("resolved_scalar", SC_LOGIC_Z);
  sc_signal_rv<width> vector("resolved_vector", vector_type(std::string(width, 'Z').c_str()));
  Observer observer("resolved_observer", scalar, vector);
  Driver driver_zero("resolved_driver_zero", scalar, vector, false);
  Driver driver_one("resolved_driver_one", scalar, vector, true);

  InResolvedProbe hook_scalar("hook_in_resolved");
  sc_signal_resolved hook_scalar_channel("hook_scalar_channel");
  hook_scalar.bind(hook_scalar_channel);
  InRvProbe<width> hook_vector("hook_in_rv");
  sc_signal_rv<width> hook_vector_channel("hook_vector_channel");
  hook_vector.bind(hook_vector_channel);

  sc_trace_file* trace = sc_create_vcd_trace_file("native_resolved_ports_public");
  trace->set_time_unit(1, SC_NS);
  sc_trace(trace, scalar, "scalar");
  sc_trace(trace, vector, "vector137");
  sc_start(2, SC_NS);
  sc_close_vcd_trace_file(trace);

  if (scalar_matrix.size() != 10 || vector_matrix.size() != 10) return 11;
  if (elaboration_count(0) != 1 || elaboration_count(1) != 1) return 12;
  if (destruction_count(0) != 1 || destruction_count(1) != 1) return 13;
  const std::vector<std::pair<std::string, std::string>> expected_scalar{{"0 s", "X"}, {"1 ns", "Z"}};
  const std::vector<std::pair<std::string, std::string>> expected_vector{{"0 s", std::string(width - 1, '0') + "X"},
                                                                         {"1 ns", std::string(width, 'Z')}};
  if (observer.scalar_values != expected_scalar || observer.vector_values != expected_vector) return 14;

  std::cout << "RESOLVED_PORT_CTORS " << scalar_matrix.size() << '|' << vector_matrix.size() << '\n';
  std::cout << "RESOLVED_PORT_HOOKS " << elaboration_count(0) << '|' << elaboration_count(1) << '\n';
  std::cout << "RESOLVED_PORT_DESTRUCTION " << destruction_count(0) << '|' << destruction_count(1) << '\n';
  std::cout << "RESOLVED_PORT_SAMPLE scalar:";
  for (std::size_t index = 0; index < observer.scalar_values.size(); ++index)
  {
    if (index) std::cout << ',';
    std::cout << observer.scalar_values[index].first << ':' << observer.scalar_values[index].second;
  }
  std::cout << "|vector137:";
  for (std::size_t index = 0; index < observer.vector_values.size(); ++index)
  {
    if (index) std::cout << ',';
    std::cout << observer.vector_values[index].first << ':' << observer.vector_values[index].second;
  }
  std::cout << '\n';
  std::cout << "RESOLVED_PORT_WIDE " << width << ' ' << vector.read().to_string() << '\n';
  std::cout << "RESOLVED_PORTS_OK\n";
  while (!scalar_ports.empty()) scalar_ports.pop_back();
  while (!vector_ports.empty()) vector_ports.pop_back();
  scalar_input_parents.clear();
  scalar_inout_parents.clear();
  vector_input_parents.clear();
  vector_inout_parents.clear();
  sc_stop();
  return 0;
}
