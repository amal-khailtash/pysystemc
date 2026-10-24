/** @file sc_core_module_probe.h
 *  @brief Compile-time and bounded native probes for the sc_module API.
 *
 *  The adapters borrow every module, signal, port, and event passed to them.
 *  Call the blocking helpers only from a live SystemC process; no Python
 *  callbacks are made here and no GIL state is acquired or released.
 */
#ifndef PYSC_SC_CORE_MODULE_PROBE_H
#define PYSC_SC_CORE_MODULE_PROBE_H

#include <stdexcept>
#include <string>
#include <systemc>
#include <type_traits>

namespace sc_core_module_probe
{
/** @brief Derived probe exposing protected module APIs to native checks. */
class ModuleProbe : public sc_core::sc_module
{
 public:
  /** @brief Construct a probe in the active SystemC module-name scope. */
  explicit ModuleProbe(sc_core::sc_module_name name) : sc_module(name)
  {
  }

  /** @brief Destroy the probe using the native virtual module destructor. */
  ~ModuleProbe() override = default;

  using sc_module::at_negedge;
  using sc_module::at_posedge;
  using sc_module::halt;
  using sc_module::next_trigger;
  using sc_module::timed_out;
  using sc_module::wait;

  /** @brief Return this module's live native simulation context. */
  sc_core::sc_simcontext* context()
  {
    return sc_get_curr_simcontext();
  }

  /** @brief Borrow the protected default sensitivity builder. */
  sc_core::sc_sensitive& sensitivity()
  {
    return sensitive;
  }

  /** @brief Borrow the protected positive-edge sensitivity builder. */
  sc_core::sc_sensitive_pos& positive_sensitivity()
  {
    return sensitive_pos;
  }

  /** @brief Borrow the protected negative-edge sensitivity builder. */
  sc_core::sc_sensitive_neg& negative_sensitivity()
  {
    return sensitive_neg;
  }

  /** @brief Call native end_module at the end of a derived constructor. */
  void finish_module()
  {
    end_module();
  }

  /** @brief Apply the native no-initialization policy to the current method. */
  void suppress_initialization()
  {
    dont_initialize();
  }

  /** @brief Configure the active native thread's requested stack size. */
  void stack_size(std::size_t size)
  {
    set_stack_size(size);
  }

  /** @brief Invoke all eight native reset overloads to check overload selection.
   *  @param in Boolean input port borrowed for this call.
   *  @param inout Boolean bidirectional port borrowed for this call.
   *  @param out Boolean output port borrowed for this call.
   *  @param iface Boolean signal interface borrowed for this call.
   */
  void reset_overloads(const sc_core::sc_in<bool>& in, const sc_core::sc_inout<bool>& inout,
                       const sc_core::sc_out<bool>& out, const sc_core::sc_signal_in_if<bool>& iface)
  {
    reset_signal_is(in, true);
    reset_signal_is(inout, true);
    reset_signal_is(out, true);
    reset_signal_is(iface, true);
    async_reset_signal_is(in, true);
    async_reset_signal_is(inout, true);
    async_reset_signal_is(out, true);
    async_reset_signal_is(iface, true);
  }

  /** @brief Compile every legacy positional binding operator for interfaces and ports.
   *  @param interface Native interface borrowed for this syntax probe.
   *  @param port Native parent port borrowed for this syntax probe.
   */
  void positional_binding_operators(sc_core::sc_interface& interface, sc_core::sc_port_base& port)
  {
    (*this)(interface);
    (*this)(port);
    *this << interface;
    *this << port;
    (*this, interface);
    (*this, port);
  }

  /** @brief Compile legacy sensitivity fields for their native source types.
   *  @param event Event borrowed by the module's sensitivity builder.
   *  @param interface Interface borrowed by the module's sensitivity builder.
   *  @param port Port borrowed by the module's sensitivity builder.
   *  @param finder Edge finder borrowed by the module's sensitivity builder.
   */
  void sensitivity_field_operators(sc_core::sc_process_handle process, sc_core::sc_event& event,
                                   sc_core::sc_interface& interface, sc_core::sc_port_base& port,
                                   sc_core::sc_event_finder& finder, const sc_core::sc_signal_in_if<bool>& bool_signal,
                                   const sc_core::sc_signal_in_if<sc_dt::sc_logic>& logic_signal,
                                   const sc_core::sc_in<bool>& bool_port,
                                   const sc_core::sc_in<sc_dt::sc_logic>& logic_port)
  {
    sensitive << event << interface << port << finder;
    sensitive_pos << process;
    sensitive_pos << bool_signal << logic_signal << bool_port << logic_port;
    sensitive_neg << process;
    sensitive_neg << bool_signal << logic_signal << bool_port << logic_port;
  }
};

/** @brief Native module owning six ports for legacy positional binding checks. */
class SixPortBindingModule : public sc_core::sc_module
{
 public:
  sc_core::sc_in<bool> input0;
  sc_core::sc_in<bool> input1;
  sc_core::sc_in<bool> input2;
  sc_core::sc_in<bool> input3;
  sc_core::sc_in<bool> input4;
  sc_core::sc_in<bool> input5;

  /** @brief Construct all positional targets inside their native module scope. */
  explicit SixPortBindingModule(sc_core::sc_module_name name)
    : sc_module(name)
    , input0("input0")
    , input1("input1")
    , input2("input2")
    , input3("input3")
    , input4("input4")
    , input5("input5")
  {
  }

  /** @brief Count bound input interfaces after elaboration. */
  int bound_count() const
  {
    return static_cast<int>(input0.size() != 0) + static_cast<int>(input1.size() != 0) +
           static_cast<int>(input2.size() != 0) + static_cast<int>(input3.size() != 0) +
           static_cast<int>(input4.size() != 0) + static_cast<int>(input5.size() != 0);
  }

  /** @brief Read each bound native interface in declaration order. */
  std::string values() const
  {
    std::string result;
    result += input0.read() ? '1' : '0';
    result += input1.read() ? '1' : '0';
    result += input2.read() ? '1' : '0';
    result += input3.read() ? '1' : '0';
    result += input4.read() ? '1' : '0';
    result += input5.read() ? '1' : '0';
    return result;
  }
};

/** @brief Legacy string-constructed module that closes its own hierarchy scope. */
class LegacyModule : public sc_core::sc_module
{
 public:
  inline static int destructions = 0;

  /** @brief Construct with the deprecated string overload and end its scope. */
  explicit LegacyModule(const char* name) : sc_module(name)
  {
    end_module();
  }

  /** @brief Count native destruction for the lifecycle reference. */
  ~LegacyModule() override
  {
    ++destructions;
  }
};

/** @brief Exercise six positional bindings on a single native module.
 *  @param module Child module whose ports receive bindings in declaration order.
 *  @param interface Native signal interface borrowed for all bindings.
 *  @param parent_port Native parent port borrowed for all bindings.
 */
inline void positional_binding_matrix(SixPortBindingModule& module, sc_core::sc_interface& interface,
                                      sc_core::sc_port_base& parent_port)
{
  module(interface);
  module(parent_port);
  module << interface;
  module << parent_port;
  (module, interface);
  (module, parent_port);
}

/** @brief Apply a comma-operator interface binding to a borrowed module. */
inline void positional_comma_interface(sc_core::sc_module& module, sc_core::sc_interface& interface)
{
  (module, interface);
}

/** @brief Apply a comma-operator parent-port binding to a borrowed module. */
inline void positional_comma_port(sc_core::sc_module& module, sc_core::sc_port_base& port)
{
  (module, port);
}

using WaitVoid = void (ModuleProbe::*)();
using WaitEvent = void (ModuleProbe::*)(const sc_core::sc_event&);
using WaitOr = void (ModuleProbe::*)(const sc_core::sc_event_or_list&);
using WaitAnd = void (ModuleProbe::*)(const sc_core::sc_event_and_list&);
using WaitTime = void (ModuleProbe::*)(const sc_core::sc_time&);
using WaitDouble = void (ModuleProbe::*)(double, sc_core::sc_time_unit);
using WaitTimeEvent = void (ModuleProbe::*)(const sc_core::sc_time&, const sc_core::sc_event&);
using WaitDoubleEvent = void (ModuleProbe::*)(double, sc_core::sc_time_unit, const sc_core::sc_event&);
using WaitTimeOr = void (ModuleProbe::*)(const sc_core::sc_time&, const sc_core::sc_event_or_list&);
using WaitDoubleOr = void (ModuleProbe::*)(double, sc_core::sc_time_unit, const sc_core::sc_event_or_list&);
using WaitTimeAnd = void (ModuleProbe::*)(const sc_core::sc_time&, const sc_core::sc_event_and_list&);
using WaitDoubleAnd = void (ModuleProbe::*)(double, sc_core::sc_time_unit, const sc_core::sc_event_and_list&);
using WaitCount = void (ModuleProbe::*)(int);

using TriggerVoid = void (ModuleProbe::*)();
using TriggerEvent = void (ModuleProbe::*)(const sc_core::sc_event&);
using TriggerOr = void (ModuleProbe::*)(const sc_core::sc_event_or_list&);
using TriggerAnd = void (ModuleProbe::*)(const sc_core::sc_event_and_list&);
using TriggerTime = void (ModuleProbe::*)(const sc_core::sc_time&);
using TriggerDouble = void (ModuleProbe::*)(double, sc_core::sc_time_unit);
using TriggerTimeEvent = void (ModuleProbe::*)(const sc_core::sc_time&, const sc_core::sc_event&);
using TriggerDoubleEvent = void (ModuleProbe::*)(double, sc_core::sc_time_unit, const sc_core::sc_event&);
using TriggerTimeOr = void (ModuleProbe::*)(const sc_core::sc_time&, const sc_core::sc_event_or_list&);
using TriggerDoubleOr = void (ModuleProbe::*)(double, sc_core::sc_time_unit, const sc_core::sc_event_or_list&);
using TriggerTimeAnd = void (ModuleProbe::*)(const sc_core::sc_time&, const sc_core::sc_event_and_list&);
using TriggerDoubleAnd = void (ModuleProbe::*)(double, sc_core::sc_time_unit, const sc_core::sc_event_and_list&);

/** @brief Keep all wait and next_trigger overloads addressable by exact type. */
static_assert(std::is_same<decltype(static_cast<WaitVoid>(&ModuleProbe::wait)), WaitVoid>::value, "wait()");
static_assert(std::is_same<decltype(static_cast<WaitEvent>(&ModuleProbe::wait)), WaitEvent>::value, "wait(event)");
static_assert(std::is_same<decltype(static_cast<WaitOr>(&ModuleProbe::wait)), WaitOr>::value, "wait(or)");
static_assert(std::is_same<decltype(static_cast<WaitAnd>(&ModuleProbe::wait)), WaitAnd>::value, "wait(and)");
static_assert(std::is_same<decltype(static_cast<WaitTime>(&ModuleProbe::wait)), WaitTime>::value, "wait(time)");
static_assert(std::is_same<decltype(static_cast<WaitDouble>(&ModuleProbe::wait)), WaitDouble>::value,
              "wait(double,unit)");
static_assert(std::is_same<decltype(static_cast<WaitTimeEvent>(&ModuleProbe::wait)), WaitTimeEvent>::value,
              "wait(time,event)");
static_assert(std::is_same<decltype(static_cast<WaitDoubleEvent>(&ModuleProbe::wait)), WaitDoubleEvent>::value,
              "wait(double,unit,event)");
static_assert(std::is_same<decltype(static_cast<WaitTimeOr>(&ModuleProbe::wait)), WaitTimeOr>::value, "wait(time,or)");
static_assert(std::is_same<decltype(static_cast<WaitDoubleOr>(&ModuleProbe::wait)), WaitDoubleOr>::value,
              "wait(double,unit,or)");
static_assert(std::is_same<decltype(static_cast<WaitTimeAnd>(&ModuleProbe::wait)), WaitTimeAnd>::value,
              "wait(time,and)");
static_assert(std::is_same<decltype(static_cast<WaitDoubleAnd>(&ModuleProbe::wait)), WaitDoubleAnd>::value,
              "wait(double,unit,and)");
static_assert(std::is_same<decltype(static_cast<WaitCount>(&ModuleProbe::wait)), WaitCount>::value, "wait(int)");
static_assert(std::is_same<decltype(static_cast<TriggerVoid>(&ModuleProbe::next_trigger)), TriggerVoid>::value,
              "next_trigger()");
static_assert(std::is_same<decltype(static_cast<TriggerEvent>(&ModuleProbe::next_trigger)), TriggerEvent>::value,
              "next_trigger(event)");
static_assert(std::is_same<decltype(static_cast<TriggerOr>(&ModuleProbe::next_trigger)), TriggerOr>::value,
              "next_trigger(or)");
static_assert(std::is_same<decltype(static_cast<TriggerAnd>(&ModuleProbe::next_trigger)), TriggerAnd>::value,
              "next_trigger(and)");
static_assert(std::is_same<decltype(static_cast<TriggerTime>(&ModuleProbe::next_trigger)), TriggerTime>::value,
              "next_trigger(time)");
static_assert(std::is_same<decltype(static_cast<TriggerDouble>(&ModuleProbe::next_trigger)), TriggerDouble>::value,
              "next_trigger(double,unit)");
static_assert(
  std::is_same<decltype(static_cast<TriggerTimeEvent>(&ModuleProbe::next_trigger)), TriggerTimeEvent>::value,
  "next_trigger(time,event)");
static_assert(
  std::is_same<decltype(static_cast<TriggerDoubleEvent>(&ModuleProbe::next_trigger)), TriggerDoubleEvent>::value,
  "next_trigger(double,unit,event)");
static_assert(std::is_same<decltype(static_cast<TriggerTimeOr>(&ModuleProbe::next_trigger)), TriggerTimeOr>::value,
              "next_trigger(time,or)");
static_assert(std::is_same<decltype(static_cast<TriggerDoubleOr>(&ModuleProbe::next_trigger)), TriggerDoubleOr>::value,
              "next_trigger(double,unit,or)");
static_assert(std::is_same<decltype(static_cast<TriggerTimeAnd>(&ModuleProbe::next_trigger)), TriggerTimeAnd>::value,
              "next_trigger(time,and)");
static_assert(
  std::is_same<decltype(static_cast<TriggerDoubleAnd>(&ModuleProbe::next_trigger)), TriggerDoubleAnd>::value,
  "next_trigger(double,unit,and)");

/** @brief Consume native member helpers from an active CTHREAD callback. */
template <typename BoolInterface, typename LogicInterface>
inline void exercise_thread_members(ModuleProbe& module, const BoolInterface& bool_signal,
                                    const LogicInterface& logic_signal)
{
  module.stack_size(32768);
  module.wait(sc_core::sc_time(1, sc_core::SC_NS));
  (void)module.timed_out();
  module.at_posedge(bool_signal);
  module.at_negedge(bool_signal);
  module.at_posedge(logic_signal);
  module.at_negedge(logic_signal);
  module.wait(1);
  module.halt();
}

/** @brief Invoke every non-clocked wait member and record each completion.
 *  @tparam Record Callable accepting the overload's zero-based index.
 *  @param module Module whose current SC_THREAD is suspended by the waits.
 *  @param event Native event retained by the caller until simulation ends.
 *  @param either Event OR-list retained by the caller until simulation ends.
 *  @param both Event AND-list retained by the caller until simulation ends.
 *  @param record Native recorder called once after each overload returns.
 */
template <typename Record>
inline void exercise_wait_members(ModuleProbe& module, const sc_core::sc_event& event,
                                  const sc_core::sc_event_or_list& either, const sc_core::sc_event_and_list& both,
                                  Record record)
{
  module.wait();
  record(0);
  module.wait(event);
  record(1);
  module.wait(either);
  record(2);
  module.wait(both);
  record(3);
  module.wait(sc_core::sc_time(1, sc_core::SC_NS));
  record(4);
  module.wait(1, sc_core::SC_NS);
  record(5);
  module.wait(sc_core::sc_time(2, sc_core::SC_NS), event);
  record(6);
  module.wait(2, sc_core::SC_NS, event);
  record(7);
  module.wait(sc_core::sc_time(2, sc_core::SC_NS), either);
  record(8);
  module.wait(2, sc_core::SC_NS, either);
  record(9);
  module.wait(sc_core::sc_time(2, sc_core::SC_NS), both);
  record(10);
  module.wait(2, sc_core::SC_NS, both);
  record(11);
}

/** @brief Invoke one of the twelve native next_trigger overloads. */
inline void invoke_next_trigger_member(ModuleProbe& module, int overload, const sc_core::sc_event& event,
                                       const sc_core::sc_event_or_list& either, const sc_core::sc_event_and_list& both)
{
  switch (overload)
  {
  case 0:
    module.next_trigger();
    break;
  case 1:
    module.next_trigger(event);
    break;
  case 2:
    module.next_trigger(either);
    break;
  case 3:
    module.next_trigger(both);
    break;
  case 4:
    module.next_trigger(sc_core::sc_time(1, sc_core::SC_NS));
    break;
  case 5:
    module.next_trigger(1, sc_core::SC_NS);
    break;
  case 6:
    module.next_trigger(sc_core::sc_time(2, sc_core::SC_NS), event);
    break;
  case 7:
    module.next_trigger(2, sc_core::SC_NS, event);
    break;
  case 8:
    module.next_trigger(sc_core::sc_time(2, sc_core::SC_NS), either);
    break;
  case 9:
    module.next_trigger(2, sc_core::SC_NS, either);
    break;
  case 10:
    module.next_trigger(sc_core::sc_time(2, sc_core::SC_NS), both);
    break;
  case 11:
    module.next_trigger(2, sc_core::SC_NS, both);
    break;
  default:
    throw std::out_of_range("next_trigger overload index");
  }
}

/** @brief Consume all native dynamic-trigger members by exact overload. */
inline void exercise_trigger_members(ModuleProbe& module, const sc_core::sc_event& event,
                                     const sc_core::sc_event_or_list& either, const sc_core::sc_event_and_list& both)
{
  module.next_trigger();
  module.next_trigger(event);
  module.next_trigger(either);
  module.next_trigger(both);
  module.next_trigger(sc_core::sc_time(1, sc_core::SC_NS));
  module.next_trigger(1, sc_core::SC_NS);
  module.next_trigger(sc_core::sc_time(1, sc_core::SC_NS), event);
  module.next_trigger(1, sc_core::SC_NS, event);
  module.next_trigger(sc_core::sc_time(1, sc_core::SC_NS), either);
  module.next_trigger(1, sc_core::SC_NS, either);
  module.next_trigger(sc_core::sc_time(1, sc_core::SC_NS), both);
  module.next_trigger(1, sc_core::SC_NS, both);
}

}  // namespace sc_core_module_probe
#endif
