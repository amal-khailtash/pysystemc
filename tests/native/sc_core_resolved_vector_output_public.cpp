/** @file sc_core_resolved_vector_output_public.cpp
 * @brief Native reference for resolved vector input/output ports.
 * @details Checks constructors, aliases, assignments, initialization,
 *          lifecycle hooks, invalid bindings, resolved writes and VCD output
 *          for arbitrary template widths without Python callbacks.
 */
#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>
#include <vector>

#include "sc_core_resolved_vector_output_probe.h"

using namespace sc_core;
using namespace sc_dt;
using namespace pysystemc_resolved_vector_output;

namespace
{
constexpr int wide_width = 137;
constexpr int small_width = 5;
using wide_value = sc_lv<wide_width>;
using small_value = sc_lv<small_width>;

/** @brief Expose SystemC's protected module constructor for test hierarchy. */
struct Scope : sc_module
{
  explicit Scope(sc_module_name name) : sc_module(name)
  {
  }
};

std::vector<int> report_ids;

/** @brief Copy public report IDs while their native reports are alive. */
void capture_report(const sc_report& report, const sc_actions&)
{
  report_ids.push_back(report.get_id());
}

/** @brief Make distinct deterministic resolved-vector source values. */
std::string assignment_bits(int width, int seed)
{
  static const char symbols[] = {'0', '1', 'X', 'Z'};
  std::string result(static_cast<std::size_t>(width), '0');
  for (int bit = 0; bit < width; ++bit)
  {
    result[static_cast<std::size_t>(bit)] = symbols[(bit + seed) % 4];
  }
  result.front() = seed % 2 ? '1' : '0';
  result.back() = seed % 2 ? '0' : '1';
  return result;
}

/** @brief Keep hook and destructor tests within an active module hierarchy. */
template <class Port, class Value, class Signal, class InoutPort>
std::vector<Port*> make_constructor_matrix(const std::string& prefix, std::vector<std::unique_ptr<Signal>>& channels,
                                           std::vector<std::unique_ptr<Port>>& ports,
                                           std::vector<std::unique_ptr<InoutPort>>& parents)
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
  parents.emplace_back(new InoutPort((prefix + "_parent_4").c_str()));
  auto* parent = parents.back().get();
  parent->bind(channel(4));
  retain(new Port(*parent));
  parents.emplace_back(new InoutPort((prefix + "_parent_5").c_str()));
  parent = parents.back().get();
  parent->bind(channel(5));
  retain(new Port((prefix + "_named_parent").c_str(), *parent));
  port = hold(new Port((prefix + "_this_parent_6").c_str(), channel(6)));
  retain(new Port(*port));
  port = hold(new Port((prefix + "_this_parent_7").c_str(), channel(7)));
  retain(new Port((prefix + "_named_this_parent").c_str(), *port));
  if (targets.size() != 8) throw std::runtime_error("resolved output constructor count");
  for (Port* target : targets)
  {
    if (target->bind_count() != 1) throw std::runtime_error("resolved output constructor binding");
  }
  return targets;
}

/** @brief Drive all five resolved-port assignment overloads from one process. */
template <int Width, class Port, class Value, class Signal, class InputPort, class InoutPort>
struct AssignmentDriver : sc_module
{
  std::array<Port*, 5> targets;
  std::array<Value, 5> values;
  Signal interface_source;
  Signal input_source;
  Signal inout_source;
  Signal this_source;
  InputPort input_parent;
  InoutPort inout_parent;
  Port this_parent;
  std::array<bool, 5> returned_self{};

  SC_HAS_PROCESS(AssignmentDriver);

  /** @brief Retain source interfaces/parents and register one assignment process. */
  AssignmentDriver(sc_module_name name, const std::array<Port*, 5>& targets_, const std::string& prefix)
    : sc_module(name)
    , targets(targets_)
    , values{Value(assignment_bits(Width, 0).c_str()), Value(assignment_bits(Width, 1).c_str()),
             Value(assignment_bits(Width, 2).c_str()), Value(assignment_bits(Width, 3).c_str()),
             Value(assignment_bits(Width, 4).c_str())}
    , interface_source((prefix + "_interface_source").c_str(), values[1])
    , input_source((prefix + "_input_source").c_str(), values[2])
    , inout_source((prefix + "_inout_source").c_str(), values[3])
    , this_source((prefix + "_this_source").c_str(), values[4])
    , input_parent((prefix + "_input_parent").c_str())
    , inout_parent((prefix + "_inout_parent").c_str())
    , this_parent((prefix + "_this_parent").c_str())
  {
    input_parent.bind(input_source);
    inout_parent.bind(inout_source);
    this_parent.bind(this_source);
    SC_METHOD(apply);
  }

  /** @brief Apply every overload and record whether each expression returns self. */
  void apply()
  {
    returned_self[0] = &(*targets[0] = values[0]) == targets[0];
    returned_self[1] = &(*targets[1] = static_cast<const typename Port::in_if_type&>(interface_source)) == targets[1];
    returned_self[2] = &(*targets[2] = input_parent) == targets[2];
    returned_self[3] = &(*targets[3] = inout_parent) == targets[3];
    returned_self[4] = &(*targets[4] = this_parent) == targets[4];
  }
};

/** @brief Observe actual value changes and retain their native timestamps. */
template <int Width>
struct Observer : sc_module
{
  sc_in_rv<Width> input;
  std::vector<std::pair<std::string, std::string>> transitions;

  SC_HAS_PROCESS(Observer);

  /** @brief Bind one resolved channel and register a noninitializing method. */
  Observer(sc_module_name name, sc_signal_rv<Width>& channel, const char* port_name) : sc_module(name), input(port_name)
  {
    input.bind(channel);
    SC_METHOD(sample);
    sensitive << input;
    dont_initialize();
  }

  /** @brief Capture complete vector value and current simulation time. */
  void sample()
  {
    std::ostringstream time;
    time << sc_time_stamp();
    transitions.emplace_back(time.str(), input.read().to_string());
  }
};

/** @brief Drive one resolved vector channel, then release every bit. */
template <int Width>
struct Writer : sc_module
{
  sc_out_rv<Width> output;
  sc_lv<Width> initial_value;
  bool initialized = false;

  SC_HAS_PROCESS(Writer);

  /** @brief Bind one output port and register its initial/release method. */
  Writer(sc_module_name name, sc_signal_rv<Width>& channel, const std::string& initial)
    : sc_module(name), output("output"), initial_value(initial.c_str())
  {
    output.bind(channel);
    SC_METHOD(drive);
  }

  /** @brief Drive at time zero and release at one nanosecond. */
  void drive()
  {
    if (initialized)
    {
      output.write(sc_lv<Width>(std::string(Width, 'Z').c_str()));
      return;
    }
    output.write(initial_value);
    initialized = true;
    next_trigger(1, SC_NS);
  }
};

/** @brief Check arbitrary-width aliases and native base relationships. */
template <int Width, class Port, class Base, class Value>
void check_aliases()
{
  static_assert(std::is_same<typename Port::this_type, Port>::value, "this_type alias");
  static_assert(std::is_same<typename Port::base_type, Base>::value, "base_type alias");
  static_assert(std::is_same<typename Port::data_type, Value>::value, "data_type alias");
  static_assert(std::is_same<typename Port::in_if_type, typename Base::in_if_type>::value, "in_if_type alias");
  static_assert(std::is_same<typename Port::in_port_type, typename Base::in_port_type>::value, "in_port_type alias");
  static_assert(std::is_same<typename Port::inout_if_type, typename Base::inout_if_type>::value, "inout_if_type alias");
  static_assert(std::is_same<typename Port::inout_port_type, typename Base::inout_port_type>::value,
                "inout_port_type alias");
}

/** @brief Capture invalid ordinary-signal bindings and their report IDs. */
int run_errors()
{
  sc_signal<wide_value> inout_ordinary("inout_ordinary");
  sc_signal<wide_value> out_ordinary("out_ordinary");
  std::unique_ptr<Scope> module;
  std::unique_ptr<sc_inout_rv<wide_width>> inout_port;
  std::unique_ptr<sc_out_rv<wide_width>> out_port;
  {
    sc_module_name module_name("resolved_vector_output_errors");
    module.reset(new Scope(module_name));
    inout_port.reset(new sc_inout_rv<wide_width>("bad_inout_rv"));
    out_port.reset(new sc_out_rv<wide_width>("bad_out_rv"));
    inout_port->bind(inout_ordinary);
    out_port->bind(out_ordinary);
  }
  const sc_report_handler_proc previous_handler = sc_report_handler::set_handler(&capture_report);
  const sc_actions previous_actions = sc_report_handler::set_actions(SC_ERROR, SC_DO_NOTHING);
  sc_start(SC_ZERO_TIME);
  sc_report_handler::set_actions(SC_ERROR, previous_actions);
  sc_report_handler::set_handler(previous_handler);
  if (report_ids != std::vector<int>{117, 117}) throw std::runtime_error("resolved vector output diagnostics");
  std::cout << "RESOLVED_VECTOR_OUTPUT_ERRORS 117|117\n";
  return 0;
}

/** @brief Trace constructor, assignment, initialized and resolved channels. */
template <int Width>
void trace_channels(sc_trace_file* trace, const std::string& prefix,
                    const std::vector<std::unique_ptr<sc_signal_rv<Width>>>& matrix,
                    const std::vector<std::unique_ptr<sc_signal_rv<Width>>>& assigned, sc_signal_rv<Width>& resolved,
                    sc_signal_rv<Width>& initialized)
{
  for (std::size_t index = 0; index < assigned.size(); ++index)
  {
    sc_trace(trace, *assigned[index], (prefix + "_assign_" + std::to_string(index)).c_str());
  }
  sc_trace(trace, resolved, (prefix + "_resolved").c_str());
  sc_trace(trace, initialized, (prefix + "_initialized").c_str());
  if (matrix.size() != 8) throw std::runtime_error("resolved vector constructor trace matrix");
}
}  // namespace

/** @brief Verify both output port families against SystemC behavior. */
int sc_main(int argc, char** argv)
{
  check_aliases<wide_width, sc_inout_rv<wide_width>, sc_inout<wide_value>, wide_value>();
  check_aliases<small_width, sc_inout_rv<small_width>, sc_inout<small_value>, small_value>();
  check_aliases<wide_width, sc_out_rv<wide_width>, sc_inout_rv<wide_width>, wide_value>();
  check_aliases<small_width, sc_out_rv<small_width>, sc_inout_rv<small_width>, small_value>();
  reset_counters();
  if (argc > 1 && std::string(argv[1]) == "errors") return run_errors();

  auto root_name = std::make_unique<sc_module_name>("resolved_vector_output_top");
  Scope root(*root_name);
  if (!verify_virtual_destruction()) return 10;
  const std::array<int, 4> destruction{
    {destruction_count(0), destruction_count(1), destruction_count(2), destruction_count(3)}};

  std::vector<std::unique_ptr<sc_signal_rv<wide_width>>> inout_wide_channels, out_wide_channels;
  std::vector<std::unique_ptr<sc_signal_rv<small_width>>> inout_small_channels, out_small_channels;
  std::vector<std::unique_ptr<sc_inout_rv<wide_width>>> inout_wide_ports;
  std::vector<std::unique_ptr<sc_out_rv<wide_width>>> out_wide_ports;
  std::vector<std::unique_ptr<sc_inout_rv<small_width>>> inout_small_ports;
  std::vector<std::unique_ptr<sc_out_rv<small_width>>> out_small_ports;
  std::vector<std::unique_ptr<sc_inout<wide_value>>> inout_wide_parents_a, inout_wide_parents_b;
  std::vector<std::unique_ptr<sc_inout<small_value>>> inout_small_parents_a, inout_small_parents_b;
  auto inout_wide_matrix =
    make_constructor_matrix<sc_inout_rv<wide_width>, wide_value, sc_signal_rv<wide_width>, sc_inout<wide_value>>(
      "sc_inout_rv137", inout_wide_channels, inout_wide_ports, inout_wide_parents_a);
  auto out_wide_matrix =
    make_constructor_matrix<sc_out_rv<wide_width>, wide_value, sc_signal_rv<wide_width>, sc_inout<wide_value>>(
      "sc_out_rv137", out_wide_channels, out_wide_ports, inout_wide_parents_b);
  auto inout_small_matrix =
    make_constructor_matrix<sc_inout_rv<small_width>, small_value, sc_signal_rv<small_width>, sc_inout<small_value>>(
      "sc_inout_rv5", inout_small_channels, inout_small_ports, inout_small_parents_a);
  auto out_small_matrix =
    make_constructor_matrix<sc_out_rv<small_width>, small_value, sc_signal_rv<small_width>, sc_inout<small_value>>(
      "sc_out_rv5", out_small_channels, out_small_ports, inout_small_parents_b);

  std::vector<std::unique_ptr<sc_signal_rv<wide_width>>> inout_wide_assignment_channels, out_wide_assignment_channels;
  std::vector<std::unique_ptr<sc_signal_rv<small_width>>> inout_small_assignment_channels,
    out_small_assignment_channels;
  std::array<sc_inout_rv<wide_width>*, 5> inout_wide_targets{};
  std::array<sc_out_rv<wide_width>*, 5> out_wide_targets{};
  std::array<sc_inout_rv<small_width>*, 5> inout_small_targets{};
  std::array<sc_out_rv<small_width>*, 5> out_small_targets{};
  const auto make_assignments = [](const std::string& prefix, auto& channels, auto& ports, auto& targets)
  {
    using ChannelPointer = typename std::remove_reference_t<decltype(channels)>::value_type;
    using PortPointer = typename std::remove_reference_t<decltype(targets)>::value_type;
    using Port = std::remove_pointer_t<PortPointer>;
    for (std::size_t index = 0; index < targets.size(); ++index)
    {
      channels.emplace_back(
        new typename ChannelPointer::element_type((prefix + "_assignment_" + std::to_string(index)).c_str()));
      ports.emplace_back(new Port((prefix + "_target_" + std::to_string(index)).c_str()));
      ports.back()->bind(*channels.back());
      targets[index] = ports.back().get();
    }
  };
  make_assignments("inout_wide", inout_wide_assignment_channels, inout_wide_ports, inout_wide_targets);
  make_assignments("out_wide", out_wide_assignment_channels, out_wide_ports, out_wide_targets);
  make_assignments("inout_small", inout_small_assignment_channels, inout_small_ports, inout_small_targets);
  make_assignments("out_small", out_small_assignment_channels, out_small_ports, out_small_targets);

  AssignmentDriver<wide_width, sc_inout_rv<wide_width>, wide_value, sc_signal_rv<wide_width>, sc_in<wide_value>,
                   sc_inout<wide_value>>
    inout_wide_assign("inout_wide_assign", inout_wide_targets, "inout_wide");
  AssignmentDriver<wide_width, sc_out_rv<wide_width>, wide_value, sc_signal_rv<wide_width>, sc_in<wide_value>,
                   sc_inout<wide_value>>
    out_wide_assign("out_wide_assign", out_wide_targets, "out_wide");
  AssignmentDriver<small_width, sc_inout_rv<small_width>, small_value, sc_signal_rv<small_width>, sc_in<small_value>,
                   sc_inout<small_value>>
    inout_small_assign("inout_small_assign", inout_small_targets, "inout_small");
  AssignmentDriver<small_width, sc_out_rv<small_width>, small_value, sc_signal_rv<small_width>, sc_in<small_value>,
                   sc_inout<small_value>>
    out_small_assign("out_small_assign", out_small_targets, "out_small");

  sc_signal_rv<wide_width> resolved_wide("resolved_rv137", wide_value(std::string(wide_width, 'Z').c_str()));
  sc_signal_rv<small_width> resolved_small("resolved_rv5", small_value(std::string(small_width, 'Z').c_str()));
  const auto drive_pattern = [](int width, bool ones)
  {
    std::string value(static_cast<std::size_t>(width), 'Z');
    value.front() = ones ? '1' : '0';
    value.back() = ones ? '1' : '0';
    return value;
  };
  Writer<wide_width> wide_zero("wide_zero_writer", resolved_wide, drive_pattern(wide_width, false));
  Writer<wide_width> wide_one("wide_one_writer", resolved_wide, drive_pattern(wide_width, true));
  Writer<small_width> small_zero("small_zero_writer", resolved_small, drive_pattern(small_width, false));
  Writer<small_width> small_one("small_one_writer", resolved_small, drive_pattern(small_width, true));
  Observer<wide_width> wide_observer("wide_observer", resolved_wide, "wide_observer_port");
  Observer<small_width> small_observer("small_observer", resolved_small, "small_observer_port");

  sc_signal_rv<wide_width> initialized_inout_wide("initialized_inout_rv137");
  sc_signal_rv<wide_width> initialized_out_wide("initialized_out_rv137");
  sc_signal_rv<small_width> initialized_inout_small("initialized_inout_rv5");
  sc_signal_rv<small_width> initialized_out_small("initialized_out_rv5");
  sc_signal_rv<wide_width> initialize_source_wide("initialize_source_rv137",
                                                  wide_value(assignment_bits(wide_width, 1).c_str()));
  sc_signal_rv<small_width> initialize_source_small("initialize_source_rv5",
                                                    small_value(assignment_bits(small_width, 1).c_str()));
  sc_inout_rv<wide_width> initialize_inout_wide("initialize_inout_rv137", initialized_inout_wide);
  sc_out_rv<wide_width> initialize_out_wide("initialize_out_rv137", initialized_out_wide);
  sc_inout_rv<small_width> initialize_inout_small("initialize_inout_rv5", initialized_inout_small);
  sc_out_rv<small_width> initialize_out_small("initialize_out_rv5", initialized_out_small);
  initialize_inout_wide.initialize(wide_value(assignment_bits(wide_width, 0).c_str()));
  initialize_out_wide.initialize(static_cast<const sc_signal_in_if<wide_value>&>(initialize_source_wide));
  initialize_inout_small.initialize(small_value(assignment_bits(small_width, 0).c_str()));
  initialize_out_small.initialize(static_cast<const sc_signal_in_if<small_value>&>(initialize_source_small));

  InoutRvProbe<wide_width> hook_inout_wide("hook_inout_rv137");
  OutRvProbe<wide_width> hook_out_wide("hook_out_rv137");
  InoutRvProbe<small_width> hook_inout_small("hook_inout_rv5");
  OutRvProbe<small_width> hook_out_small("hook_out_rv5");
  sc_signal_rv<wide_width> hook_inout_wide_channel("hook_inout_wide_channel"),
    hook_out_wide_channel("hook_out_wide_channel");
  sc_signal_rv<small_width> hook_inout_small_channel("hook_inout_small_channel"),
    hook_out_small_channel("hook_out_small_channel");
  hook_inout_wide.bind(hook_inout_wide_channel);
  hook_out_wide.bind(hook_out_wide_channel);
  hook_inout_small.bind(hook_inout_small_channel);
  hook_out_small.bind(hook_out_small_channel);

  sc_trace_file* trace = sc_create_vcd_trace_file("native_resolved_vector_output_public");
  trace->set_time_unit(1, SC_NS);
  trace_channels(trace, "inout137", inout_wide_channels, inout_wide_assignment_channels, resolved_wide,
                 initialized_inout_wide);
  trace_channels(trace, "out137", out_wide_channels, out_wide_assignment_channels, resolved_wide, initialized_out_wide);
  trace_channels(trace, "inout5", inout_small_channels, inout_small_assignment_channels, resolved_small,
                 initialized_inout_small);
  trace_channels(trace, "out5", out_small_channels, out_small_assignment_channels, resolved_small,
                 initialized_out_small);
  root_name.reset();
  sc_start(2, SC_NS);
  sc_close_vcd_trace_file(trace);

  const auto check_matrix = [](auto& matrix, const std::string& expected_kind)
  {
    if (matrix.size() != 8) throw std::runtime_error("resolved output matrix size");
    for (auto* port : matrix)
    {
      if (port->bind_count() != 1 || port->kind() != expected_kind)
        throw std::runtime_error("resolved output matrix metadata");
    }
  };
  check_matrix(inout_wide_matrix, "sc_inout_rv");
  check_matrix(inout_small_matrix, "sc_inout_rv");
  check_matrix(out_wide_matrix, "sc_out_rv");
  check_matrix(out_small_matrix, "sc_out_rv");
  const std::array<int, 4> hooks{
    {elaboration_count(0), elaboration_count(1), elaboration_count(2), elaboration_count(3)}};
  if (hooks != std::array<int, 4>{{1, 1, 1, 1}} || destruction != std::array<int, 4>{{1, 1, 1, 1}})
    throw std::runtime_error("resolved output lifecycle counts");
  if (!inout_wide_assign.returned_self[0] || !inout_wide_assign.returned_self[1] ||
      !inout_wide_assign.returned_self[2] || !inout_wide_assign.returned_self[3] ||
      !inout_wide_assign.returned_self[4] || !out_wide_assign.returned_self[0] || !out_wide_assign.returned_self[1] ||
      !out_wide_assign.returned_self[2] || !out_wide_assign.returned_self[3] || !out_wide_assign.returned_self[4] ||
      !inout_small_assign.returned_self[0] || !inout_small_assign.returned_self[1] ||
      !inout_small_assign.returned_self[2] || !inout_small_assign.returned_self[3] ||
      !inout_small_assign.returned_self[4] || !out_small_assign.returned_self[0] ||
      !out_small_assign.returned_self[1] || !out_small_assign.returned_self[2] || !out_small_assign.returned_self[3] ||
      !out_small_assign.returned_self[4])
    throw std::runtime_error("resolved output assignment return identity");

  const std::string resolved_wide_text = resolved_wide.read().to_string();
  const std::string resolved_small_text = resolved_small.read().to_string();
  if (resolved_wide_text != std::string(wide_width, 'Z') || resolved_small_text != std::string(small_width, 'Z'))
    throw std::runtime_error("resolved writers did not release");
  const std::string expected_wide_conflict = "X" + std::string(wide_width - 2, 'Z') + "X";
  const std::string expected_small_conflict = "X" + std::string(small_width - 2, 'Z') + "X";
  if (wide_observer.transitions !=
        std::vector<std::pair<std::string, std::string>>{{"0 s", expected_wide_conflict},
                                                         {"1 ns", std::string(wide_width, 'Z')}} ||
      small_observer.transitions != std::vector<std::pair<std::string, std::string>>{
                                      {"0 s", expected_small_conflict}, {"1 ns", std::string(small_width, 'Z')}})
    throw std::runtime_error("resolved vector samples");
  const auto check_assignments = [](const auto& fixture, const auto& channels, int width)
  {
    for (std::size_t index = 0; index < channels.size(); ++index)
    {
      const std::string expected = assignment_bits(width, static_cast<int>(index));
      if (channels[index]->read().to_string() != expected || fixture.values[index].to_string() != expected)
        throw std::runtime_error("resolved vector assignment value");
    }
  };
  check_assignments(inout_wide_assign, inout_wide_assignment_channels, wide_width);
  check_assignments(out_wide_assign, out_wide_assignment_channels, wide_width);
  check_assignments(inout_small_assign, inout_small_assignment_channels, small_width);
  check_assignments(out_small_assign, out_small_assignment_channels, small_width);

  std::cout << "RESOLVED_VECTOR_OUTPUT_CTORS " << inout_wide_matrix.size() << '|' << out_wide_matrix.size() << '|'
            << inout_small_matrix.size() << '|' << out_small_matrix.size() << '\n';
  std::cout << "RESOLVED_VECTOR_OUTPUT_HOOKS " << hooks[0] << '|' << hooks[1] << '|' << hooks[2] << '|' << hooks[3]
            << '\n';
  std::cout << "RESOLVED_VECTOR_OUTPUT_DESTRUCTION " << destruction[0] << '|' << destruction[1] << '|' << destruction[2]
            << '|' << destruction[3] << '\n';
  std::cout << "RESOLVED_VECTOR_OUTPUT_ASSIGNMENTS 20\n";
  std::cout << "RESOLVED_VECTOR_OUTPUT_SAMPLES wide:" << wide_observer.transitions[0].second << ','
            << wide_observer.transitions[1].second << "|small:" << small_observer.transitions[0].second << ','
            << small_observer.transitions[1].second << '\n';
  std::cout << "RESOLVED_VECTOR_OUTPUT_INITIALIZE " << initialized_inout_wide.read().to_string() << '|'
            << initialized_out_wide.read().to_string() << '|' << initialized_inout_small.read().to_string() << '|'
            << initialized_out_small.read().to_string() << '\n';
  std::cout << "RESOLVED_VECTOR_OUTPUT_OK\n";
  return 0;
}
