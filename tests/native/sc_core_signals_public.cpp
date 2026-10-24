/** @file sc_core_signals_public.cpp
 * @brief Native reference for the Python sc_signal and sc_signal_t matrix.
 * @details Every instance belongs to one independent SystemC kernel process.
 * The signal_t fixtures add no callback ownership or Python state. The output
 * port and trace file are owned by Model and released before the kernel exits.
 */
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "sc_core_signals_probe.h"

namespace
{

using sc_core::SC_MANY_WRITERS;
using sc_core::SC_ONE_WRITER;
using sc_core::SC_UNCHECKED_WRITERS;
using sc_dt::sc_biguint;
using sc_dt::sc_logic;

/** @brief Fail the native comparator without relying on release assert flags. */
void require(bool condition, const char* message = "native signal contract failed")
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Format a bool with SystemC's native stream representation. */
std::string value_text(bool value)
{
  return value ? "1" : "0";
}

/** @brief Format a logic value with SystemC's native stream representation. */
std::string value_text(const sc_logic& value)
{
  return std::string(1, value.to_char());
}

/** @brief Format any other streamable native signal value. */
template <class T>
std::string value_text(const T& value)
{
  std::ostringstream stream;
  stream << value;
  return stream.str();
}

template <class T, sc_core::sc_writer_policy POL>
class SignalPair
{
 public:
  std::string name;
  T initial;
  signal_probe::signal_t_access<T, POL> adapter;
  sc_core::sc_signal<T, POL> signal;
  sc_core::sc_out<T> adapter_output;
  sc_core::sc_out<T> signal_output;

  /** @brief Construct matching adapter and public channels under one module. */
  SignalPair(const char* name_, const T& initial_)
    : name(name_)
    , initial(initial_)
    , adapter((name + "_base").c_str(), initial)
    , signal(name.c_str(), initial)
    , adapter_output((name + "_base_output").c_str())
    , signal_output((name + "_output").c_str())
  {
    adapter_output.bind(adapter);
    signal_output.bind(signal);
  }

  /** @brief Exercise stream helpers, event aliases and direct deprecated trace. */
  void check_api(sc_core::sc_trace_file* trace) const
  {
    for_each_channel(
      [&](const auto& channel, const std::string& channel_name)
      {
        require(channel.kind() == std::string("sc_signal"));
        require(channel.read() == initial);
        require(channel.get_data_ref() == initial);
        require(channel.get_new_value() == initial);
        require(!channel.event());
        require(&channel.default_event() == &channel.value_changed_event());
        std::ostringstream stream;
        channel.print(stream);
        require(stream.str() == value_text(initial));
        stream.str("");
        stream.clear();
        channel.dump(stream);
        require(stream.str().find(channel_name) != std::string::npos);
        channel.trace(trace);
      });
  }

  /** @brief Trace the ordinary channel through the public native trace API. */
  void add_trace(sc_core::sc_trace_file* trace) const
  {
    sc_core::sc_trace(trace, signal, name.c_str());
  }

  /** @brief Queue the first sample through native assignment overloads. */
  void first(const T& value)
  {
    adapter = value;
    signal = value;
  }

  /** @brief Queue the later sample through the public write methods. */
  void later(const T& value)
  {
    adapter.write(value);
    signal_output.write(value);
  }

  /** @brief Select self and input-interface assignment overloads independently. */
  void assign_from_channels()
  {
    adapter.operator=(static_cast<const sc_core::sc_signal_t<T, POL>&>(adapter));
    signal.operator=(static_cast<const sc_core::sc_signal<T, POL>&>(signal));
    adapter = static_cast<const sc_core::sc_signal_in_if<T>&>(signal);
    signal = static_cast<const sc_core::sc_signal_in_if<T>&>(adapter);
  }

  /** @brief Emit a matching snapshot for the adapter and public channel. */
  void emit(const char* stage) const
  {
    emit_one(name, "signal_t", stage, adapter, initial);
    emit_one(name, "signal", stage, signal, initial);
  }

 private:
  /** @brief Apply one observer to the adapter and public channel. */
  template <class Fn>
  void for_each_channel(Fn&& function) const
  {
    function(adapter, name + "_base");
    function(signal, name);
  }

  /** @brief Emit and validate a channel snapshot after a native update. */
  template <class Channel>
  static void emit_one(const std::string& case_name, const char* surface, const char* stage, const Channel& channel,
                       const T& initial_value)
  {
    std::string edges = "-|-";
    if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, sc_logic>)
    {
      edges =
        std::to_string(static_cast<int>(channel.posedge())) + "|" + std::to_string(static_cast<int>(channel.negedge()));
      require(&channel.posedge_event() != &channel.negedge_event());
    }
    const int policy = static_cast<int>(channel.get_writer_policy());
    const std::string current_value = value_text(channel.read());
    const std::string next_value = value_text(channel.get_new_value());
    const int did_event = static_cast<int>(channel.event());
    require(channel.kind() == std::string("sc_signal"));
    require(&channel.default_event() == &channel.value_changed_event());
    require(channel.get_data_ref() == channel.read());
    require(&signal_probe::value_conversion<T, POL>(channel) == &channel.get_data_ref());
    require(channel.event());
    std::cout << "SIGNAL|" << case_name << "|" << surface << "|" << stage << "|" << policy << "|"
              << value_text(initial_value) << "|" << current_value << "|" << next_value << "|" << did_event << "|"
              << edges << '\n';
  }
};

template <class T>
class ConstructorSet
{
 public:
  sc_core::sc_signal<T> defaulted;
  sc_core::sc_signal<T> named;
  sc_core::sc_signal<T> initialized;
  T initial;
  std::string name;

  /** @brief Construct all three public signal constructor forms. */
  ConstructorSet(const char* name, const T& initial_)
    : defaulted()
    , named((std::string(name) + "_named").c_str())
    , initialized((std::string(name) + "_initialized").c_str(), initial_)
    , initial(initial_)
    , name(name)
  {
  }

  /** @brief Verify native default, named and initial-value constructor results. */
  void verify(const char* type_name) const
  {
    emit_one(type_name, "default", defaulted, T());
    emit_one(type_name, "named", named, T());
    emit_one(type_name, "initial", initialized, initial);
    require(std::string(named.name()).find(name + "_named") != std::string::npos);
    require(std::string(initialized.name()).find(name + "_initialized") != std::string::npos);
  }

 private:
  /** @brief Verify and report one constructor result. */
  static void emit_one(const char* type_name, const char* form, const sc_core::sc_signal<T>& signal, const T& expected)
  {
    require(signal.kind() == std::string("sc_signal"));
    require(signal.read() == expected);
    require(signal.get_writer_policy() == SC_ONE_WRITER);
    std::cout << "SIGNAL_CTOR|" << type_name << "|" << form << "|" << static_cast<int>(signal.get_writer_policy())
              << "|" << value_text(signal.read()) << '\n';
  }
};

class Model : public sc_core::sc_module
{
  SC_HAS_PROCESS(Model);

 public:
  SignalPair<int, SC_ONE_WRITER> int_one;
  SignalPair<bool, SC_ONE_WRITER> bool_one;
  SignalPair<bool, SC_MANY_WRITERS> bool_many;
  SignalPair<bool, SC_UNCHECKED_WRITERS> bool_unchecked;
  SignalPair<sc_logic, SC_ONE_WRITER> logic_one;
  SignalPair<sc_logic, SC_MANY_WRITERS> logic_many;
  SignalPair<sc_logic, SC_UNCHECKED_WRITERS> logic_unchecked;
  using wide_type = sc_biguint<137>;
  SignalPair<wide_type, SC_ONE_WRITER> wide;
  ConstructorSet<int> ctor_int;
  ConstructorSet<bool> ctor_bool;
  ConstructorSet<sc_logic> ctor_logic;
  sc_core::sc_trace_file* trace;

  /** @brief Own all channels, connect the output port and prepare VCD tracing. */
  explicit Model(sc_core::sc_module_name name)
    : sc_module(name)
    , int_one("int_one", 5)
    , bool_one("bool_one", false)
    , bool_many("bool_many", false)
    , bool_unchecked("bool_unchecked", false)
    , logic_one("logic_one", sc_logic('0'))
    , logic_many("logic_many", sc_logic('0'))
    , logic_unchecked("logic_unchecked", sc_logic('0'))
    , wide("wide_signal", wide_type(0))
    , ctor_int("ctor_int", 19)
    , ctor_bool("ctor_bool", true)
    , ctor_logic("ctor_logic", sc_logic('1'))
    , trace(sc_core::sc_create_vcd_trace_file("signals_public"))
  {
    using generic_signal = sc_core::sc_signal<int, SC_ONE_WRITER>;
    static_assert(std::is_same_v<generic_signal::base_type, sc_core::sc_signal_t<int, SC_ONE_WRITER>>);
    static_assert(std::is_same_v<generic_signal::if_type, sc_core::sc_signal_inout_if<int>>);
    static_assert(std::is_same_v<generic_signal::policy_type, sc_core::sc_writer_policy_check<SC_ONE_WRITER>>);
    static_assert(std::is_same_v<generic_signal::this_type, generic_signal>);
    static_assert(std::is_same_v<generic_signal::value_type, int>);

    trace->set_time_unit(1, sc_core::SC_NS);
    ctor_int.verify("int");
    ctor_bool.verify("bool");
    ctor_logic.verify("logic");
    int_one.check_api(trace);
    bool_one.check_api(trace);
    bool_many.check_api(trace);
    bool_unchecked.check_api(trace);
    logic_one.check_api(trace);
    logic_many.check_api(trace);
    logic_unchecked.check_api(trace);
    wide.check_api(trace);
    int_one.add_trace(trace);
    bool_one.add_trace(trace);
    bool_many.add_trace(trace);
    bool_unchecked.add_trace(trace);
    logic_one.add_trace(trace);
    logic_many.add_trace(trace);
    logic_unchecked.add_trace(trace);
    sc_core::sc_trace(trace, wide.signal, "wide_sample");
    SC_THREAD(drive);
  }

  /** @brief Run rise/fall transitions and verify assignment/event semantics. */
  void drive()
  {
    int_one.first(11);
    bool_one.first(true);
    bool_many.first(true);
    bool_unchecked.first(true);
    logic_one.first(sc_logic('1'));
    logic_many.first(sc_logic('1'));
    logic_unchecked.first(sc_logic('1'));
    wide.first(wide_type("87112285931760246646623899502532662133001"));
    wait(sc_core::SC_ZERO_TIME);
    int_one.emit("rise");
    bool_one.emit("rise");
    bool_many.emit("rise");
    bool_unchecked.emit("rise");
    logic_one.emit("rise");
    logic_many.emit("rise");
    logic_unchecked.emit("rise");
    wide.emit("rise");

    int_one.assign_from_channels();
    bool_one.assign_from_channels();
    bool_many.assign_from_channels();
    bool_unchecked.assign_from_channels();
    logic_one.assign_from_channels();
    logic_many.assign_from_channels();
    logic_unchecked.assign_from_channels();
    require(int_one.adapter.get_new_value() == 11);
    require(int_one.signal.get_new_value() == 11);
    wait(sc_core::SC_ZERO_TIME);
    require(int_one.adapter.read() == 11 && int_one.signal.read() == 11);
    require(!int_one.adapter.event() && !int_one.signal.event());
    require(bool_one.adapter.read() && bool_one.signal.read());
    require(bool_many.adapter.read() && bool_many.signal.read());
    require(bool_unchecked.adapter.read() && bool_unchecked.signal.read());
    require(logic_one.adapter.read() == sc_logic('1') && logic_one.signal.read() == sc_logic('1'));
    require(logic_many.adapter.read() == sc_logic('1') && logic_many.signal.read() == sc_logic('1'));
    require(logic_unchecked.adapter.read() == sc_logic('1') && logic_unchecked.signal.read() == sc_logic('1'));
    std::cout << "SIGNAL_ASSIGN|interface|self|seven-pairs-stable\n";

    wait(1, sc_core::SC_NS);
    int_one.adapter.write(13);
    int_one.signal_output.write(13);
    bool_one.later(false);
    bool_many.later(false);
    bool_unchecked.later(false);
    logic_one.later(sc_logic('0'));
    logic_many.later(sc_logic('0'));
    logic_unchecked.later(sc_logic('0'));
    wide.later(wide_type("87112285931760246646623899502532662133007"));
    wait(sc_core::SC_ZERO_TIME);
    int_one.emit("fall");
    bool_one.emit("fall");
    bool_many.emit("fall");
    bool_unchecked.emit("fall");
    logic_one.emit("fall");
    logic_many.emit("fall");
    logic_unchecked.emit("fall");
    wide.emit("fall");
    std::cout << "SIGNAL_TIME|" << sc_core::sc_time_stamp() << "|" << sc_core::sc_delta_count() << '\n';
    wait(sc_core::SC_ZERO_TIME);
    wait(1, sc_core::SC_NS);
  }
};

/** @brief Test duplicate output registration for one native writer policy. */
template <sc_core::sc_writer_policy POL>
class PortPolicyCheck : public sc_core::sc_module
{
 public:
  sc_core::sc_signal<bool, POL> signal;
  sc_core::sc_out<bool> first;
  sc_core::sc_out<bool> second;

  /** @brief Bind two output ports to test this signal writer policy. */
  explicit PortPolicyCheck(sc_core::sc_module_name name)
    : sc_module(name), signal("policy_signal"), first("first_output"), second("second_output")
  {
    first.bind(signal);
    second.bind(signal);
  }
};

}  // namespace

/** @brief Run the native reference matrix or one isolated port-policy case. */
int sc_main(int argc, char** argv)
{
  if (argc > 1 && std::string(argv[1]).rfind("port-", 0) == 0)
  {
    const std::string policy = std::string(argv[1]).substr(5);
    int error_id = 0;
    try
    {
      if (policy == "one")
      {
        PortPolicyCheck<SC_ONE_WRITER> model("signal_port_policy");
        sc_core::sc_start(sc_core::SC_ZERO_TIME);
      }
      else if (policy == "many")
      {
        PortPolicyCheck<SC_MANY_WRITERS> model("signal_port_policy");
        sc_core::sc_start(sc_core::SC_ZERO_TIME);
      }
      else if (policy == "unchecked")
      {
        PortPolicyCheck<SC_UNCHECKED_WRITERS> model("signal_port_policy");
        sc_core::sc_start(sc_core::SC_ZERO_TIME);
      }
      else
      {
        throw std::runtime_error("unknown port policy");
      }
    }
    catch (const sc_core::sc_report& report)
    {
      error_id = report.get_id();
    }
    if (policy == "one")
    {
      require(error_id == 115, "SC_ONE_WRITER duplicate output port did not report E115");
      std::cout << "SIGNAL_PORT|one|duplicate-error|" << error_id << '\n';
    }
    else
    {
      require(error_id == 0, "multiple-output registration unexpectedly failed");
      std::cout << "SIGNAL_PORT|" << policy << "|multiple-outputs-allowed|0\n";
    }
    return 0;
  }

  Model model("signals_public");
  sc_core::sc_start(2, sc_core::SC_NS);
  sc_core::sc_close_vcd_trace_file(model.trace);
  return 0;
}
