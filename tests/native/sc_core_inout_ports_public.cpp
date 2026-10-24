/** @file sc_core_inout_ports_public.cpp
 * @brief Native reference for generic, bool and logic inout/output ports.
 */
#include <array>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <vector>

#include "sc_core_inout_ports_probe.h"

using namespace sc_core;
using namespace sc_dt;
using namespace pysystemc_inout_ports;

namespace
{
/** @brief Publicly construct a native module for the standalone error case. */
struct ErrorModule : sc_module
{
  /** @brief Initialize the SystemC module base from its scoped name. */
  explicit ErrorModule(sc_module_name name) : sc_module(name)
  {
  }
};

/** @brief Make a stable 137-bit value matching the Python reference pattern. */
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

/** @brief Construct native bool, sc_logic or 137-bit vector samples. */
template <class T>
T make_value(int seed)
{
  if constexpr (std::is_same_v<T, bool>)
  {
    return seed % 2 != 0;
  }
  else if constexpr (std::is_same_v<T, sc_logic>)
  {
    constexpr char symbols[] = {'0', '1', 'X', 'Z'};
    return sc_logic(symbols[seed % 4]);
  }
  else
  {
    return T(vector_value(seed).c_str());
  }
}

/** @brief Format values identically to the Python model. */
template <class T>
std::string value_text(const T& value)
{
  std::ostringstream stream;
  stream << value;
  return stream.str();
}

/** @brief Construct all eight native inout/output constructors for one type. */
template <class T, class Port>
struct ConstructorMatrix
{
  std::array<std::unique_ptr<sc_signal<T>>, 8> channels;
  std::array<std::unique_ptr<Port>, 8> ports;
  std::array<std::unique_ptr<sc_inout<T>>, 2> inout_parents;
  std::array<std::unique_ptr<Port>, 2> same_type_parents;

  /** @brief Build default, named, interface, base-port and same-type forms. */
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
      const int channel_index = index + 4;
      const auto parent_name = prefix + "_ctor_parent_" + std::to_string(channel_index);
      inout_parents[static_cast<std::size_t>(index)] = std::make_unique<sc_inout<T>>(parent_name.c_str());
      inout_parents[static_cast<std::size_t>(index)]->bind(*channels[static_cast<std::size_t>(channel_index)]);
      if (index == 0)
      {
        ports[static_cast<std::size_t>(channel_index)] = std::make_unique<Port>(*inout_parents[0]);
      }
      else
      {
        ports[static_cast<std::size_t>(channel_index)] =
          std::make_unique<Port>((prefix + "_ctor_named_parent").c_str(), *inout_parents[1]);
      }
    }

    for (int index = 0; index < 2; ++index)
    {
      const int channel_index = index + 6;
      const auto parent_name = prefix + "_ctor_this_parent_" + std::to_string(channel_index);
      same_type_parents[static_cast<std::size_t>(index)] = std::make_unique<Port>(parent_name.c_str());
      same_type_parents[static_cast<std::size_t>(index)]->bind(*channels[static_cast<std::size_t>(channel_index)]);
      if (index == 0)
      {
        ports[static_cast<std::size_t>(channel_index)] = std::make_unique<Port>(*same_type_parents[0]);
      }
      else
      {
        ports[static_cast<std::size_t>(channel_index)] =
          std::make_unique<Port>((prefix + "_ctor_named_this_parent").c_str(), *same_type_parents[1]);
      }
    }
  }

  /** @brief Count constructor forms that bound once and report the native kind. */
  bool valid(const char* expected_kind) const
  {
    for (const auto& port : ports)
    {
      if (port->bind_count() != 1 || std::string(port->kind()) != expected_kind) return false;
    }
    return true;
  }
};

/** @brief Build five source and destination types for operator= dispatch. */
template <class T, class Port>
struct AssignmentMatrix
{
  std::array<std::unique_ptr<sc_signal<T>>, 5> target_channels;
  std::array<std::unique_ptr<Port>, 5> targets;
  sc_signal<T> interface_source;
  sc_signal<T> input_source_channel;
  sc_in<T> input_source;
  sc_signal<T> inout_source_channel;
  sc_inout<T> inout_source;
  sc_signal<T> self_source_channel;
  Port self_source;
  std::array<bool, 5> return_self{};

  /** @brief Construct each native assignment source with a distinct value. */
  explicit AssignmentMatrix(const std::string& prefix)
    : interface_source((prefix + "_assign_interface").c_str(), make_value<T>(1))
    , input_source_channel((prefix + "_assign_input_channel").c_str(), make_value<T>(2))
    , input_source((prefix + "_assign_input_port").c_str())
    , inout_source_channel((prefix + "_assign_inout_channel").c_str(), make_value<T>(3))
    , inout_source((prefix + "_assign_inout_port").c_str())
    , self_source_channel((prefix + "_assign_self_channel").c_str(), make_value<T>(4))
    , self_source((prefix + "_assign_this_port").c_str())
  {
    input_source.bind(input_source_channel);
    inout_source.bind(inout_source_channel);
    self_source.bind(self_source_channel);
    for (std::size_t index = 0; index < targets.size(); ++index)
    {
      const auto channel_name = prefix + "_assign_target_" + std::to_string(index);
      const auto port_name = prefix + "_assign_target_port_" + std::to_string(index);
      target_channels[index] = std::make_unique<sc_signal<T>>(channel_name.c_str(), make_value<T>(0));
      targets[index] = std::make_unique<Port>(port_name.c_str());
      targets[index]->bind(*target_channels[index]);
    }
  }

  /** @brief Exercise data, interface, input, inout and same-type assignments. */
  void apply()
  {
    return_self[0] = &((*targets[0]) = make_value<T>(0)) == targets[0].get();
    return_self[1] = &((*targets[1]) = interface_source) == targets[1].get();
    return_self[2] = &((*targets[2]) = input_source) == targets[2].get();
    return_self[3] = &((*targets[3]) = inout_source) == targets[3].get();
    return_self[4] = &((*targets[4]) = self_source) == targets[4].get();
  }

  /** @brief Return all copied channel values after the scheduled update. */
  std::string values() const
  {
    std::string result;
    for (std::size_t index = 0; index < target_channels.size(); ++index)
    {
      if (index != 0) result += ',';
      result += value_text(target_channels[index]->read());
    }
    return result;
  }

  /** @brief True if all five assignment overloads returned their destination. */
  bool all_return_self() const
  {
    for (bool value : return_self)
    {
      if (!value) return false;
    }
    return true;
  }
};

/** @brief Own initialized inout ports and one lifecycle-probed output port. */
template <class T, int Family>
struct InitializationSet
{
  sc_signal<T> data_channel;
  sc_signal<T> interface_channel;
  sc_signal<T> interface_source;
  sc_signal<T> out_channel;
  InoutProbe<T, Family> data_port;
  InoutProbe<T, Family> interface_port;
  OutProbe<T, Family> out_port;

  /** @brief Cache data/interface initialization before binding each inout. */
  explicit InitializationSet(const std::string& prefix)
    : data_channel((prefix + "_initialize_data_channel").c_str(), make_value<T>(0))
    , interface_channel((prefix + "_initialize_interface_channel").c_str(), make_value<T>(0))
    , interface_source((prefix + "_initialize_source_channel").c_str(), make_value<T>(3))
    , out_channel((prefix + "_out_lifecycle_channel").c_str(), make_value<T>(0))
    , data_port((prefix + "_initialize_data").c_str())
    , interface_port((prefix + "_initialize_interface").c_str())
    , out_port((prefix + "_out_lifecycle").c_str())
  {
    data_port.initialize(make_value<T>(2));
    data_port.bind(data_channel);
    interface_port.initialize(interface_source);
    interface_port.bind(interface_channel);
    out_port.bind(out_channel);
  }

  /** @brief Register and discard one protected trace-record group, then add trace. */
  void trace_records(sc_trace_file* trace, const std::string& prefix)
  {
    data_port.add_trace_internal(trace, prefix + "_discarded");
    if (data_port.trace_record_count() != 1) throw std::runtime_error("trace record setup failed");
    data_port.discard_trace_records();
    if (data_port.trace_record_count() != 0) throw std::runtime_error("protected trace cleanup failed");
    data_port.add_trace(trace, prefix + "_member_add_trace");
  }
};

/** @brief Own one independently driven inout/output channel pair. */
template <class T>
struct WavePair
{
  sc_signal<T> inout_channel;
  sc_inout<T> inout_port;
  sc_signal<T> out_channel;
  sc_out<T> out_port;

  /** @brief Construct ports unbound so free sc_trace sees prebind storage. */
  explicit WavePair(const std::string& prefix)
    : inout_channel((prefix + "_wave_inout_channel").c_str(), make_value<T>(0))
    , inout_port((prefix + "_wave_inout").c_str())
    , out_channel((prefix + "_wave_out_channel").c_str(), make_value<T>(0))
    , out_port((prefix + "_wave_out").c_str())
  {
  }

  /** @brief Bind both deferred trace ports after trace registration. */
  void bind()
  {
    inout_port.bind(inout_channel);
    out_port.bind(out_channel);
  }

  /** @brief Write the same sample through two distinct native ports. */
  void write(int seed)
  {
    if constexpr (std::is_same_v<T, sc_logic>)
    {
      const int value_seed = seed == 2 ? 0 : seed;
      const auto value = make_value<T>(value_seed);
      inout_port.write(value);
      out_port.write(value);
    }
    else
    {
      const auto value = make_value<T>(seed);
      inout_port.write(value);
      out_port.write(value);
    }
  }
};

/** @brief Register all port rows in one native hierarchy and simulate events. */
struct PortReference : sc_module
{
  ConstructorMatrix<sc_bv<137>, sc_inout<sc_bv<137>>> inout_wide{"wide"};
  ConstructorMatrix<bool, sc_inout<bool>> inout_bool{"bool"};
  ConstructorMatrix<sc_logic, sc_inout<sc_logic>> inout_logic{"logic"};
  ConstructorMatrix<sc_bv<137>, sc_out<sc_bv<137>>> out_wide{"wide_out"};
  ConstructorMatrix<bool, sc_out<bool>> out_bool{"bool_out"};
  ConstructorMatrix<sc_logic, sc_out<sc_logic>> out_logic{"logic_out"};

  AssignmentMatrix<sc_bv<137>, sc_inout<sc_bv<137>>> inout_assign_wide{"wide"};
  AssignmentMatrix<bool, sc_inout<bool>> inout_assign_bool{"bool"};
  AssignmentMatrix<sc_logic, sc_inout<sc_logic>> inout_assign_logic{"logic"};
  AssignmentMatrix<sc_bv<137>, sc_out<sc_bv<137>>> out_assign_wide{"wide_out"};
  AssignmentMatrix<bool, sc_out<bool>> out_assign_bool{"bool_out"};
  AssignmentMatrix<sc_logic, sc_out<sc_logic>> out_assign_logic{"logic_out"};

  InitializationSet<sc_bv<137>, 0> init_wide{"wide"};
  InitializationSet<bool, 1> init_bool{"bool"};
  InitializationSet<sc_logic, 2> init_logic{"logic"};
  WavePair<sc_bv<137>> wave_wide{"wide"};
  WavePair<bool> wave_bool{"bool"};
  WavePair<sc_logic> wave_logic{"logic"};
  int phase = 0;
  std::vector<std::string> events;
  bool finder_cache = false;

  SC_HAS_PROCESS(PortReference);

  /** @brief Build test families, direct lifetimes and all event-sensitive methods. */
  explicit PortReference(sc_module_name name) : sc_module(name)
  {
    const bool destruction_verified = verify_virtual_destruction();
    if (!destruction_verified) throw std::runtime_error("virtual port destruction failed");
    finder_cache = &wave_bool.inout_port.pos() == &wave_bool.inout_port.pos() &&
                   &wave_bool.inout_port.neg() == &wave_bool.inout_port.neg() &&
                   &wave_bool.inout_port.value_changed() == &wave_bool.inout_port.value_changed() &&
                   &wave_logic.inout_port.pos() == &wave_logic.inout_port.pos() &&
                   &wave_logic.inout_port.neg() == &wave_logic.inout_port.neg() &&
                   &wave_logic.inout_port.value_changed() == &wave_logic.inout_port.value_changed() &&
                   &wave_wide.inout_port.value_changed() == &wave_wide.inout_port.value_changed();

    SC_METHOD(drive);
    SC_METHOD(on_wide_change);
    sensitive << wave_wide.inout_port.value_changed();
    dont_initialize();
    SC_METHOD(on_bool_change);
    sensitive << wave_bool.inout_port.value_changed();
    dont_initialize();
    SC_METHOD(on_bool_pos);
    sensitive << wave_bool.inout_port.pos();
    dont_initialize();
    SC_METHOD(on_bool_neg);
    sensitive << wave_bool.inout_port.neg();
    dont_initialize();
    SC_METHOD(on_logic_change);
    sensitive << wave_logic.inout_port.value_changed();
    dont_initialize();
    SC_METHOD(on_logic_pos);
    sensitive << wave_logic.inout_port.pos();
    dont_initialize();
    SC_METHOD(on_logic_neg);
    sensitive << wave_logic.inout_port.neg();
    dont_initialize();
  }

  /** @brief Bind all deferred waveform ports after their trace calls. */
  void bind_wave_ports()
  {
    wave_wide.bind();
    wave_bool.bind();
    wave_logic.bind();
  }

  /** @brief Apply 30 assignment overloads, write initial values and reschedule. */
  void drive()
  {
    if (phase == 0)
    {
      inout_assign_wide.apply();
      inout_assign_bool.apply();
      inout_assign_logic.apply();
      out_assign_wide.apply();
      out_assign_bool.apply();
      out_assign_logic.apply();
      wave_wide.write(1);
      wave_bool.write(1);
      wave_logic.write(1);
      phase = 1;
      next_trigger(1, SC_NS);
    }
    else if (phase == 1)
    {
      wave_wide.write(2);
      wave_bool.write(2);
      wave_logic.write(2);
      phase = 2;
    }
  }

  /** @brief Append the wide value and port event at the current time. */
  void on_wide_change()
  {
    events.push_back(event_text("wide", "value", wave_wide.inout_port, false));
  }

  /** @brief Append the bool value-change event. */
  void on_bool_change()
  {
    events.push_back(event_text("bool", "value", wave_bool.inout_port, true));
  }

  /** @brief Append a positive bool edge. */
  void on_bool_pos()
  {
    events.push_back(event_text("bool", "pos", wave_bool.inout_port, true));
  }

  /** @brief Append a negative bool edge. */
  void on_bool_neg()
  {
    events.push_back(event_text("bool", "neg", wave_bool.inout_port, true));
  }

  /** @brief Append the logic value-change event. */
  void on_logic_change()
  {
    events.push_back(event_text("logic", "value", wave_logic.inout_port, true));
  }

  /** @brief Append a positive logic edge. */
  void on_logic_pos()
  {
    events.push_back(event_text("logic", "pos", wave_logic.inout_port, true));
  }

  /** @brief Append a negative logic edge. */
  void on_logic_neg()
  {
    events.push_back(event_text("logic", "neg", wave_logic.inout_port, true));
  }

  /** @brief Normalize one event record for native/Python comparison. */
  template <class T>
  std::string event_text(const char* label, const char* kind, const sc_inout<T>& port, bool edges) const
  {
    std::ostringstream stream;
    stream << "INOUT_PORT_EVENT|" << label << '|' << kind << '|' << sc_time_stamp() << '|' << value_text(port.read())
           << '|' << port.event() << '|';
    if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, sc_logic>)
    {
      stream << (edges && port.posedge()) << '|' << (edges && port.negedge());
    }
    else
    {
      stream << false << '|' << false;
    }
    return stream.str();
  }
};

/** @brief Print native constructor, assignment, initialization and event records. */
void print_reference(const PortReference& model)
{
  const auto print_assignment = [](const char* family, const char* label, const auto& matrix)
  {
    std::cout << "INOUT_PORT_ASSIGN|" << family << '|' << label << '|' << matrix.values() << "|self=";
    for (bool value : matrix.return_self) std::cout << (value ? '1' : '0');
    std::cout << '\n';
  };
  print_assignment("inout", "wide", model.inout_assign_wide);
  print_assignment("out", "wide", model.out_assign_wide);
  print_assignment("inout", "bool", model.inout_assign_bool);
  print_assignment("out", "bool", model.out_assign_bool);
  print_assignment("inout", "logic", model.inout_assign_logic);
  print_assignment("out", "logic", model.out_assign_logic);
  std::cout << "INOUT_PORT_CTORS|wide:8/8,bool:8/8,logic:8/8\n";
  std::cout << "INOUT_PORT_ALIASES|wide,bool,logic|out-base=inout\n";
  std::cout << "INOUT_PORT_INITIALIZE|wide:" << value_text(model.init_wide.data_port.read()) << '/'
            << value_text(model.init_wide.interface_port.read()) << ",bool:" << model.init_bool.data_port.read() << '/'
            << model.init_bool.interface_port.read() << ",logic:" << model.init_logic.data_port.read() << '/'
            << model.init_logic.interface_port.read() << '\n';
  std::cout << "INOUT_PORT_FINDERS|bool:cached,logic:cached,wide:cached\n";
  for (const auto& event : model.events) std::cout << event << '\n';
  std::cout << "INOUT_PORT_LIFECYCLE|hooks=2,2,2;out=1,1,1;virtual_delete=1x6\n";
  std::cout << "INOUT_PORT_TIME|" << sc_time_stamp() << '\n';
}
}  // namespace

/** @brief Run a standalone error case or the full native port reference. */
int sc_main(int argc, char** argv)
{
  reset_lifecycle_counts();
  if (argc > 1 && std::string(argv[1]) == "--errors")
  {
    sc_module_name module_name("inout_port_errors");
    ErrorModule module(module_name);
    sc_inout<bool> port("unbound_inout");
    try
    {
      sc_start(SC_ZERO_TIME);
    }
    catch (const sc_report& report)
    {
      if (std::string(report.what()).find("complete binding failed") == std::string::npos ||
          std::string(report.what()).find("inout_port_errors.unbound_inout") == std::string::npos)
        return 2;
      std::cout << "INOUT_PORT_ERRORS|unbound-required\n";
      return 0;
    }
    return 3;
  }

  sc_module_name module_name("inout_ports_top");
  auto model_owner = std::make_unique<PortReference>(module_name);
  auto& model = *model_owner;
  if (!model.finder_cache || inout_elaboration_count(0) != 0 || inout_elaboration_count(1) != 0 ||
      inout_elaboration_count(2) != 0 || out_elaboration_count(0) != 0 || out_elaboration_count(1) != 0 ||
      out_elaboration_count(2) != 0)
    return 4;
  sc_trace_file* trace = sc_create_vcd_trace_file("sc_core_inout_ports_public");
  trace->set_time_unit(1, SC_NS);
  sc_trace(trace, model.wave_wide.inout_port, "wide_inout");
  sc_trace(trace, model.wave_wide.out_port, "wide_out");
  sc_trace(trace, model.wave_bool.inout_port, "bool_inout");
  sc_trace(trace, model.wave_bool.out_port, "bool_out");
  sc_trace(trace, model.wave_logic.inout_port, "logic_inout");
  sc_trace(trace, model.wave_logic.out_port, "logic_out");
  model.init_wide.trace_records(trace, "wide");
  model.init_bool.trace_records(trace, "bool");
  model.init_logic.trace_records(trace, "logic");
  model.bind_wave_ports();

  for (auto* port :
       {static_cast<sc_port_base*>(&model.wave_wide.inout_port), static_cast<sc_port_base*>(&model.wave_wide.out_port),
        static_cast<sc_port_base*>(&model.wave_bool.inout_port), static_cast<sc_port_base*>(&model.wave_bool.out_port),
        static_cast<sc_port_base*>(&model.wave_logic.inout_port),
        static_cast<sc_port_base*>(&model.wave_logic.out_port)})
  {
    if (port->bind_count() != 1) return 5;
  }
  if (conversion_value_address(model.wave_wide.inout_port) != read_value_address(model.wave_wide.inout_port) ||
      conversion_value_address(model.wave_bool.inout_port) != read_value_address(model.wave_bool.inout_port) ||
      conversion_value_address(model.wave_logic.inout_port) != read_value_address(model.wave_logic.inout_port) ||
      conversion_value_address(model.wave_wide.out_port) != read_value_address(model.wave_wide.out_port) ||
      conversion_value_address(model.wave_bool.out_port) != read_value_address(model.wave_bool.out_port) ||
      conversion_value_address(model.wave_logic.out_port) != read_value_address(model.wave_logic.out_port))
    return 6;
  if (!model.inout_wide.valid("sc_inout") || !model.inout_bool.valid("sc_inout") ||
      !model.inout_logic.valid("sc_inout") || !model.out_wide.valid("sc_out") || !model.out_bool.valid("sc_out") ||
      !model.out_logic.valid("sc_out"))
    return 7;

  sc_start(2, SC_NS);
  sc_close_vcd_trace_file(trace);
  if (inout_elaboration_count(0) != 2 || inout_elaboration_count(1) != 2 || inout_elaboration_count(2) != 2 ||
      out_elaboration_count(0) != 1 || out_elaboration_count(1) != 1 || out_elaboration_count(2) != 1)
    return 8;
  print_reference(model);
  return 0;
}
