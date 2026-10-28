/** @file sc_core_input_ports_public.cpp
 *  @brief Native reference for generic, bool and logic sc_in ports.
 */
#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <type_traits>
#include <vector>

#include "sc_core_input_ports_probe.h"

using namespace sc_core;
using namespace sc_dt;
using namespace pysystemc_input_ports;

namespace
{
/** @brief Return deterministic 137-bit values matching the Python reference. */
std::string vector_value(int seed)
{
  std::string value(137, '0');
  for (int index = 0; index < 137; ++index)
  {
    if ((index * 11 + seed * 7) % 19 < 8) value[static_cast<std::size_t>(index)] = '1';
  }
  value[0] = seed % 2 ? '1' : '0';
  value.back() = seed % 2 ? '0' : '1';
  return value;
}

/** @brief Create the family-specific bool, logic or vector sample. */
template <class T>
T make_value(int seed)
{
  if constexpr (std::is_same<T, bool>::value)
  {
    return seed % 2 != 0;
  }
  else if constexpr (std::is_same<T, sc_logic>::value)
  {
    constexpr char symbols[] = {'0', '1', 'X', 'Z'};
    return sc_logic(symbols[seed % 4]);
  }
  else
  {
    return T(vector_value(seed).c_str());
  }
}

/** @brief Format a signal value using its standard SystemC stream spelling. */
template <class T>
std::string value_text(const T& value)
{
  std::ostringstream stream;
  stream << value;
  return stream.str();
}

/** @brief Instantiate all ten sc_in constructors for one data type. */
template <class T, class Port>
struct ConstructorMatrix
{
  using input_interface = sc_signal_in_if<T>;
  using inout_interface = sc_signal_inout_if<T>;
  static_assert(std::is_same<typename Port::data_type, T>::value, "sc_in data_type alias");
  static_assert(std::is_same<typename Port::if_type, input_interface>::value, "sc_in if_type alias");
  static_assert(std::is_same<typename Port::in_if_type, input_interface>::value, "sc_in in_if_type alias");
  static_assert(std::is_same<typename Port::inout_if_type, inout_interface>::value, "sc_in inout_if_type alias");
  static_assert(std::is_same<typename Port::base_type, sc_port<input_interface, 1, SC_ONE_OR_MORE_BOUND>>::value,
                "sc_in base_type alias");
  static_assert(std::is_same<typename Port::in_port_type, typename Port::base_type>::value, "sc_in in_port_type alias");
  static_assert(std::is_same<typename Port::inout_port_type, sc_port<inout_interface, 1, SC_ONE_OR_MORE_BOUND>>::value,
                "sc_in inout_port_type alias");
  static_assert(std::is_same<typename Port::base_port_type, typename Port::base_type::port_type>::value,
                "sc_in base_port_type alias");
  static_assert(std::is_same<typename Port::this_type, Port>::value, "sc_in this_type alias");
  static_assert(!std::is_copy_constructible<Port>::value, "sc_in copy construction is disabled");
  static_assert(!std::is_copy_assignable<Port>::value, "sc_in copy assignment is disabled");
  std::array<std::unique_ptr<sc_signal<T>>, 10> channels;
  std::array<std::unique_ptr<Port>, 10> ports;
  std::array<std::unique_ptr<typename Port::in_port_type>, 2> input_parents;
  std::array<std::unique_ptr<typename Port::inout_port_type>, 2> inout_parents;
  std::array<std::unique_ptr<Port>, 2> same_type_parents;

  /** @brief Build default, named, interface, input, inout and self-parent forms. */
  explicit ConstructorMatrix(const std::string& prefix)
  {
    for (std::size_t index = 0; index < channels.size(); ++index)
    {
      const auto channel_name = prefix + "_ctor_signal_" + std::to_string(index);
      channels[index] = std::make_unique<sc_signal<T>>(channel_name.c_str(), make_value<T>(0));
    }
    ports[0] = std::make_unique<Port>();
    ports[0]->bind(*channels[0]);
    ports[1] = std::make_unique<Port>((prefix + "_ctor_named").c_str());
    ports[1]->bind(*channels[1]);
    ports[2] = std::make_unique<Port>(*channels[2]);
    ports[3] = std::make_unique<Port>((prefix + "_ctor_named_interface").c_str(), *channels[3]);
    for (int index = 0; index < 2; ++index)
    {
      const int channel = index + 4;
      const auto parent_name = prefix + "_ctor_parent_" + std::to_string(channel);
      input_parents[static_cast<std::size_t>(index)] =
        std::make_unique<typename Port::in_port_type>(parent_name.c_str());
      input_parents[static_cast<std::size_t>(index)]->bind(*channels[static_cast<std::size_t>(channel)]);
      ports[static_cast<std::size_t>(channel)] =
        index == 0 ? std::make_unique<Port>(*input_parents[0])
                   : std::make_unique<Port>((prefix + "_ctor_named_parent").c_str(), *input_parents[1]);
    }
    for (int index = 0; index < 2; ++index)
    {
      const int channel = index + 6;
      const auto parent_name = prefix + "_ctor_inout_parent_" + std::to_string(channel);
      inout_parents[static_cast<std::size_t>(index)] =
        std::make_unique<typename Port::inout_port_type>(parent_name.c_str());
      inout_parents[static_cast<std::size_t>(index)]->bind(*channels[static_cast<std::size_t>(channel)]);
      ports[static_cast<std::size_t>(channel)] =
        index == 0 ? std::make_unique<Port>(*inout_parents[0])
                   : std::make_unique<Port>((prefix + "_ctor_named_inout_parent").c_str(), *inout_parents[1]);
    }
    for (int index = 0; index < 2; ++index)
    {
      const int channel = index + 8;
      const auto parent_name = prefix + "_ctor_this_parent_" + std::to_string(channel);
      same_type_parents[static_cast<std::size_t>(index)] = std::make_unique<Port>(parent_name.c_str());
      same_type_parents[static_cast<std::size_t>(index)]->bind(*channels[static_cast<std::size_t>(channel)]);
      ports[static_cast<std::size_t>(channel)] =
        index == 0 ? std::make_unique<Port>(*same_type_parents[0])
                   : std::make_unique<Port>((prefix + "_ctor_named_this").c_str(), *same_type_parents[1]);
    }
  }

  /** @brief Confirm every constructor result is a bound native sc_in. */
  bool valid() const
  {
    for (const auto& port : ports)
    {
      if (port->bind_count() != 1 || std::string(port->kind()) != "sc_in") return false;
    }
    return true;
  }
};

/** @brief Exercise public sc_in interface and parent bind/call overloads. */
template <class T>
struct BindingMatrix
{
  using in_type = sc_in<T>;
  using in_port_type = typename in_type::in_port_type;
  using inout_port_type = typename in_type::inout_port_type;
  std::array<sc_signal<T>, 7> channels;
  std::array<in_type, 7> targets;
  std::array<in_port_type, 2> input_parents;
  std::array<inout_port_type, 2> inout_parents;

  /** @brief Bind two interfaces and both parent kinds through both public calls. */
  explicit BindingMatrix(const std::string& prefix)
    : channels{sc_signal<T>((prefix + "_interface_bind").c_str(), make_value<T>(0)),
               sc_signal<T>((prefix + "_interface_const_bind").c_str(), make_value<T>(0)),
               sc_signal<T>((prefix + "_interface_call").c_str(), make_value<T>(0)),
               sc_signal<T>((prefix + "_parent_bind").c_str(), make_value<T>(0)),
               sc_signal<T>((prefix + "_parent_call").c_str(), make_value<T>(0)),
               sc_signal<T>((prefix + "_inout_bind").c_str(), make_value<T>(0)),
               sc_signal<T>((prefix + "_inout_call").c_str(), make_value<T>(0))}
    , targets{in_type((prefix + "_bind_interface").c_str()),    in_type((prefix + "_bind_const_interface").c_str()),
              in_type((prefix + "_call_interface").c_str()),    in_type((prefix + "_bind_input_parent").c_str()),
              in_type((prefix + "_call_input_parent").c_str()), in_type((prefix + "_bind_inout_parent").c_str()),
              in_type((prefix + "_call_inout_parent").c_str())}
    , input_parents{in_port_type((prefix + "_input_parent_bind").c_str()),
                    in_port_type((prefix + "_input_parent_call").c_str())}
    , inout_parents{inout_port_type((prefix + "_inout_parent_bind").c_str()),
                    inout_port_type((prefix + "_inout_parent_call").c_str())}
  {
    targets[0].bind(static_cast<typename in_type::in_if_type&>(channels[0]));
    targets[1].bind(static_cast<const typename in_type::in_if_type&>(channels[1]));
    targets[2](static_cast<const typename in_type::in_if_type&>(channels[2]));
    input_parents[0].bind(channels[3]);
    input_parents[1].bind(channels[4]);
    inout_parents[0].bind(channels[5]);
    inout_parents[1].bind(channels[6]);
    targets[3].bind(input_parents[0]);
    targets[4](input_parents[1]);
    targets[5].bind(inout_parents[0]);
    targets[6](inout_parents[1]);
  }

  /** @brief Verify direct interfaces and deferred parent connections. */
  bool valid()
  {
    for (std::size_t index = 0; index < targets.size(); ++index)
    {
      if (targets[index].bind_count() != 1 || std::string(targets[index].kind()) != "sc_in") return false;
      if ((index < 3 && targets[index].size() != 1) || (index >= 3 && targets[index].size() != 0)) return false;
    }
    return true;
  }
};

/** @brief Own a delayed trace port whose pending records exercise sc_in cleanup. */
template <class T, int Family>
struct TraceSet
{
  sc_signal<T> channel;
  std::unique_ptr<InputProbe<T, Family>> port;

  /** @brief Create the channel and a bound derived input within its module. */
  explicit TraceSet(const std::string& prefix)
    : channel((prefix + "_member_trace_channel").c_str(), make_value<T>(0))
    , port(new InputProbe<T, Family>((prefix + "_member_trace_input").c_str()))
  {
    port->bind(channel);
  }
};

/** @brief Own a regular input port and signal for deferred waveform tracing. */
template <class T>
struct Wave
{
  sc_signal<T> channel;
  sc_in<T> port;

  /** @brief Construct an initially unbound channel/port pair. */
  explicit Wave(const std::string& prefix)
    : channel((prefix + "_wave_channel").c_str(), make_value<T>(0)), port((prefix + "_wave_input").c_str())
  {
  }

  /** @brief Bind after trace registration to test native deferred trace records. */
  void bind()
  {
    port.bind(channel);
  }
};

/** @brief Assemble all three sc_in families and observe scheduled changes. */
struct InputReference : sc_module
{
  ConstructorMatrix<sc_bv<137>, sc_in<sc_bv<137>>> constructors_wide{"wide"};
  ConstructorMatrix<bool, sc_in<bool>> constructors_bool{"bool"};
  ConstructorMatrix<sc_logic, sc_in<sc_logic>> constructors_logic{"logic"};
  BindingMatrix<sc_bv<137>> bindings_wide{"wide_bindings"};
  BindingMatrix<bool> bindings_bool{"bool_bindings"};
  BindingMatrix<sc_logic> bindings_logic{"logic_bindings"};
  TraceSet<sc_bv<137>, 0> trace_wide{"wide"};
  TraceSet<bool, 1> trace_bool{"bool"};
  TraceSet<sc_logic, 2> trace_logic{"logic"};
  Wave<sc_bv<137>> wave_wide{"wide"};
  Wave<bool> wave_bool{"bool"};
  Wave<sc_logic> wave_logic{"logic"};
  int phase = 0;
  bool finders_cached = false;
  std::vector<std::string> events;

  SC_HAS_PROCESS(InputReference);

  /** @brief Register input constructor, binder and event checks in the hierarchy. */
  explicit InputReference(sc_module_name name) : sc_module(name)
  {
    finders_cached = &wave_wide.port.value_changed() == &wave_wide.port.value_changed() &&
                     &wave_bool.port.value_changed() == &wave_bool.port.value_changed() &&
                     &wave_bool.port.pos() == &wave_bool.port.pos() && &wave_bool.port.neg() == &wave_bool.port.neg() &&
                     &wave_logic.port.value_changed() == &wave_logic.port.value_changed() &&
                     &wave_logic.port.pos() == &wave_logic.port.pos() &&
                     &wave_logic.port.neg() == &wave_logic.port.neg();
    SC_METHOD(on_wide_change);
    sensitive << wave_wide.port.value_changed();
    dont_initialize();
    SC_METHOD(on_bool_change);
    sensitive << wave_bool.port.value_changed();
    dont_initialize();
    SC_METHOD(on_bool_pos);
    sensitive << wave_bool.port.pos();
    dont_initialize();
    SC_METHOD(on_bool_neg);
    sensitive << wave_bool.port.neg();
    dont_initialize();
    SC_METHOD(on_logic_change);
    sensitive << wave_logic.port.value_changed();
    dont_initialize();
    SC_METHOD(on_logic_pos);
    sensitive << wave_logic.port.pos();
    dont_initialize();
    SC_METHOD(on_logic_neg);
    sensitive << wave_logic.port.neg();
    dont_initialize();
    SC_METHOD(drive);
  }

  /** @brief Add traces before binding and exercise the deprecated member API. */
  void add_traces(sc_trace_file* trace)
  {
    sc_trace(trace, wave_wide.port, "wide_input");
    sc_trace(trace, wave_bool.port, "bool_input");
    sc_trace(trace, wave_logic.port, "logic_input");
    trace_wide.port->add_trace_internal(trace, "wide_discarded");
    trace_bool.port->add_trace_internal(trace, "bool_discarded");
    trace_logic.port->add_trace_internal(trace, "logic_discarded");
    trace_wide.port->discard_trace_records();
    trace_bool.port->discard_trace_records();
    trace_logic.port->discard_trace_records();
    trace_wide.port->add_trace(trace, "wide_member");
    trace_bool.port->add_trace(trace, "bool_member");
    trace_logic.port->add_trace(trace, "logic_member");
  }

  /** @brief Bind waveform ports after prebinding trace registrations. */
  void bind_waves()
  {
    wave_wide.bind();
    wave_bool.bind();
    wave_logic.bind();
  }

  /** @brief Write two scheduled transitions to each signal. */
  void drive()
  {
    if (phase == 0)
    {
      wave_wide.channel.write(make_value<sc_bv<137>>(1));
      wave_bool.channel.write(make_value<bool>(1));
      wave_logic.channel.write(make_value<sc_logic>(1));
      phase = 1;
      next_trigger(1, SC_NS);
    }
    else if (phase == 1)
    {
      wave_wide.channel.write(make_value<sc_bv<137>>(2));
      wave_bool.channel.write(make_value<bool>(0));
      wave_logic.channel.write(make_value<sc_logic>(0));
      phase = 2;
    }
  }

  /** @brief Record read/event/edge state for one callback invocation. */
  template <class T>
  void observe(const char* label, const char* kind, const sc_in<T>& port, bool edges)
  {
    bool positive = false;
    bool negative = false;
    if constexpr (std::is_same<T, bool>::value || std::is_same<T, sc_logic>::value)
    {
      if (edges)
      {
        positive = port.posedge();
        negative = port.negedge();
      }
    }
    std::ostringstream stream;
    stream << "INPUT_PORT_EVENT|" << label << '|' << kind << '|' << sc_time_stamp() << '|' << value_text(port.read())
           << '|' << port.event() << '|' << positive << '|' << negative;
    events.push_back(stream.str());
  }
  void on_wide_change()
  {
    observe("wide", "value", wave_wide.port, false);
  }
  void on_bool_change()
  {
    observe("bool", "value", wave_bool.port, true);
  }
  void on_bool_pos()
  {
    observe("bool", "pos", wave_bool.port, true);
  }
  void on_bool_neg()
  {
    observe("bool", "neg", wave_bool.port, true);
  }
  void on_logic_change()
  {
    observe("logic", "value", wave_logic.port, true);
  }
  void on_logic_pos()
  {
    observe("logic", "pos", wave_logic.port, true);
  }
  void on_logic_neg()
  {
    observe("logic", "neg", wave_logic.port, true);
  }
};

/** @brief Run an unbound required-input error case in its own kernel. */
int run_errors()
{
  sc_module_name module_name("input_port_errors");
  struct ErrorModule : sc_module
  {
    explicit ErrorModule(sc_module_name name) : sc_module(name)
    {
    }
  } module(module_name);
  sc_in<bool> port("unbound_input");
  try
  {
    sc_start(SC_ZERO_TIME);
  }
  catch (const sc_report& report)
  {
    const std::string message(report.what());
    if (message.find("complete binding failed") == std::string::npos ||
        message.find("input_port_errors.unbound_input") == std::string::npos)
      return 2;
    std::cout << "INPUT_PORT_ERRORS|unbound-required\n";
    return 0;
  }
  return 3;
}

}  // namespace

/** @brief Run a native error case or compare all sc_in families against Python. */
int sc_main(int argc, char** argv)
{
  reset_lifecycle_counts();
  if (argc > 1 && std::string(argv[1]) == "--errors") return run_errors();

  sc_module_name module_name("input_ports_top");
  std::unique_ptr<InputReference> model_owner(new InputReference(module_name));
  InputReference& model = *model_owner;
  if (!model.finders_cached || !model.constructors_wide.valid() || !model.constructors_bool.valid() ||
      !model.constructors_logic.valid() || !model.bindings_wide.valid() || !model.bindings_bool.valid() ||
      !model.bindings_logic.valid() || !verify_virtual_destruction())
    return 4;

  sc_trace_file* trace = sc_create_vcd_trace_file("sc_core_input_ports_public");
  trace->set_time_unit(1, SC_NS);
  model.add_traces(trace);
  model.bind_waves();
  if (conversion_value_address(model.wave_wide.port) != read_value_address(model.wave_wide.port) ||
      conversion_value_address(model.wave_bool.port) != read_value_address(model.wave_bool.port) ||
      conversion_value_address(model.wave_logic.port) != read_value_address(model.wave_logic.port))
    return 5;
  if (value_text(model.wave_wide.port.read()) != value_text(make_value<sc_bv<137>>(0)) ||
      model.wave_bool.port.read() != make_value<bool>(0) || model.wave_logic.port.read() != make_value<sc_logic>(0))
    return 6;

  sc_start(2, SC_NS);
  sc_close_vcd_trace_file(trace);
  if (elaboration_count(0) != 1 || elaboration_count(1) != 1 || elaboration_count(2) != 1 ||
      model.trace_wide.port->trace_record_count() != 0 || model.trace_bool.port->trace_record_count() != 0 ||
      model.trace_logic.port->trace_record_count() != 0)
    return 7;
  model.trace_wide.port.reset();
  model.trace_bool.port.reset();
  model.trace_logic.port.reset();
  if (destruction_count(0) != 2 || destruction_count(1) != 2 || destruction_count(2) != 2) return 8;

  std::cout << "INPUT_PORT_CTORS|wide:10/10,bool:10/10,logic:10/10\n";
  std::cout << "INPUT_PORT_BIND|wide:7/7,bool:7/7,logic:7/7\n";
  std::cout << "INPUT_PORT_ALIASES|wide,bool,logic\n";
  std::cout << "INPUT_PORT_FINDERS|bool:cached,logic:cached,wide:cached\n";
  for (const auto& event : model.events) std::cout << event << '\n';
  std::cout << "INPUT_PORT_LIFECYCLE|elab=1,1,1;virtual_delete=2,2,2\n";
  std::cout << "INPUT_PORT_TIME|" << sc_time_stamp() << '\n';
  return 0;
}
