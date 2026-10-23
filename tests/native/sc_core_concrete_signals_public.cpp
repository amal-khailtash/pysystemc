/** @file sc_core_concrete_signals_public.cpp
 * @brief Native reference for concrete buffer and resolved signal families.
 * @details Runs an isolated SystemC kernel, compares constructor and assignment
 *          forms, observes updates through ports, and writes a nonempty VCD.
 *          Native channels own their values; no Python callback or GIL state is
 *          involved in this reference executable.
 */
#include <iostream>
#include <memory>
#include <string>
#include <systemc>

#include "sc_core_concrete_signals_probe.h"

using namespace sc_core;
using namespace sc_dt;
using namespace pysystemc_concrete_signals;

namespace
{
/** @brief Observe value-change events for the public channel interfaces. */
struct SignalObserver : sc_module
{
  sc_in<bool> buffer_port{"buffer"};
  sc_in<bool> boolean_port{"boolean"};
  sc_in<sc_logic> resolved_port{"resolved"};
  sc_in<sc_lv<16>> vector_port{"vector"};
  std::vector<bool> buffer_events, boolean_events, resolved_events, vector_events;
  std::vector<sc_dt::uint64> buffer_deltas, boolean_deltas, resolved_deltas, vector_deltas;

  SC_HAS_PROCESS(SignalObserver);

  /** @brief Bind channels and register noninitializing methods. */
  SignalObserver(sc_module_name name, BufferProbe<bool>& buffer, sc_signal<bool>& boolean, ResolvedProbe& resolved,
                 ResolvedVectorProbe<16>& vector)
    : sc_module(name)
  {
    buffer_port(buffer);
    boolean_port(boolean);
    resolved_port(resolved);
    vector_port(vector);
    SC_METHOD(on_buffer);
    sensitive << buffer_port;
    dont_initialize();
    SC_METHOD(on_boolean);
    sensitive << boolean_port;
    dont_initialize();
    SC_METHOD(on_resolved);
    sensitive << resolved_port;
    dont_initialize();
    SC_METHOD(on_vector);
    sensitive << vector_port;
    dont_initialize();
  }

  /** @brief Record buffer update notifications. */
  void on_buffer()
  {
    buffer_events.push_back(buffer_port.event());
    buffer_deltas.push_back(sc_delta_count());
  }

  /** @brief Record boolean value-change events. */
  void on_boolean()
  {
    boolean_events.push_back(boolean_port.event());
    boolean_deltas.push_back(sc_delta_count());
  }

  /** @brief Record resolved scalar value-change events. */
  void on_resolved()
  {
    resolved_events.push_back(resolved_port.event());
    resolved_deltas.push_back(sc_delta_count());
  }

  /** @brief Record resolved vector value-change events. */
  void on_vector()
  {
    vector_events.push_back(vector_port.event());
    vector_deltas.push_back(sc_delta_count());
  }
};

/** @brief Produce conflicting values from separate native writer processes. */
struct MultiWriter : sc_module
{
  ResolvedProbe& resolved;
  ResolvedVectorProbe<16>& vector;
  SC_HAS_PROCESS(MultiWriter);

  /** @brief Register two independent writers to the many-writer channels. */
  MultiWriter(sc_module_name name, ResolvedProbe& resolved_, ResolvedVectorProbe<16>& vector_)
    : sc_module(name), resolved(resolved_), vector(vector_)
  {
    SC_METHOD(writer_zero);
    SC_METHOD(writer_one);
  }

  /** @brief Drive zero and a vector with a low-order one. */
  void writer_zero()
  {
    resolved.write(SC_LOGIC_0);
    vector.write(sc_lv<16>("0000000000000010"));
  }

  /** @brief Drive one and a vector with low-order Z bits. */
  void writer_one()
  {
    resolved.write(SC_LOGIC_1);
    vector.write(sc_lv<16>("00000000000000ZZ"));
  }
};

/** @brief Print a stable constructor record for a native channel. */
template <typename Signal>
void emit_constructor(const char* family, int index, const Signal& signal)
{
  std::cout << "CONCRETE_CTOR " << family << ' ' << index << ' ' << signal.kind() << ' ' << signal.read() << '\n';
}

/** @brief Print a stable assignment record after native updates have run. */
template <typename Signal>
void emit_assignments(const char* family, const Signal& value, const Signal& interface, const Signal& self)
{
  std::cout << "CONCRETE_ASSIGN " << family << ' ' << value.read() << '|' << interface.read() << '|' << self.read()
            << '\n';
}
}  // namespace

/** @brief Execute the native constructor, assignment, event, writer and trace cases. */
int sc_main(int, char**)
{
  reset_rv_destruction_count();
  const int destruction_before = rv_destruction_count();
  {
    std::unique_ptr<sc_signal_rv<16>> owning_base(new ResolvedVectorDestructionProbe<16>("rv_destruction_probe"));
  }
  const int destruction_after = rv_destruction_count();
  if (destruction_after != destruction_before + 1) return 20;

  sc_buffer<bool> buffer_default;
  sc_buffer<bool> buffer_named("buffer_named");
  sc_buffer<bool> buffer_initial("buffer_initial", true);
  sc_signal_resolved resolved_default;
  sc_signal_resolved resolved_named("resolved_named");
  sc_signal_resolved resolved_initial("resolved_initial", SC_LOGIC_Z);
  sc_signal_rv<16> rv_default;
  sc_signal_rv<16> rv_named("rv_named");
  sc_signal_rv<16> rv_initial("rv_initial", sc_lv<16>("01XZ"));

  sc_signal<bool> buffer_interface_source("buffer_if_source", true);
  sc_buffer<bool> buffer_self_source("buffer_self_source", true);
  sc_buffer<bool> buffer_assign_value("buffer_assign_value");
  sc_buffer<bool> buffer_assign_interface("buffer_assign_interface");
  sc_buffer<bool> buffer_assign_self("buffer_assign_self");
  buffer_assign_value = true;
  buffer_assign_interface = buffer_interface_source;
  buffer_assign_self = buffer_self_source;

  sc_signal<sc_logic> resolved_interface_source("resolved_if_source", SC_LOGIC_1);
  sc_signal_resolved resolved_self_source("resolved_self_source", SC_LOGIC_Z);
  sc_signal_resolved resolved_assign_value("resolved_assign_value");
  sc_signal_resolved resolved_assign_interface("resolved_assign_interface");
  sc_signal_resolved resolved_assign_self("resolved_assign_self");
  resolved_assign_value = SC_LOGIC_0;
  resolved_assign_interface = resolved_interface_source;
  resolved_assign_self = resolved_self_source;

  sc_signal<sc_lv<16>> rv_interface_source("rv_if_source", sc_lv<16>("1010"));
  sc_signal_rv<16> rv_self_source("rv_self_source", sc_lv<16>("ZZ01"));
  sc_signal_rv<16> rv_assign_value("rv_assign_value");
  sc_signal_rv<16> rv_assign_interface("rv_assign_interface");
  sc_signal_rv<16> rv_assign_self("rv_assign_self");
  rv_assign_value = sc_lv<16>("01XZ");
  rv_assign_interface = rv_interface_source;
  rv_assign_self = rv_self_source;

  BufferProbe<bool> buffer("buffer_observed");
  sc_signal<bool> boolean("boolean_observed");
  ResolvedProbe resolved("resolved_observed", SC_LOGIC_Z);
  ResolvedVectorProbe<16> vector("vector_observed", sc_lv<16>("0"));
  SignalObserver observer("signal_observer", buffer, boolean, resolved, vector);
  ResolvedProbe resolved_multi("resolved_multi", SC_LOGIC_Z);
  ResolvedVectorProbe<16> vector_multi("vector_multi", sc_lv<16>("Z"));
  MultiWriter multi_writer("multi_writer", resolved_multi, vector_multi);

  std::string wide_initial;
  std::string wide_next;
  while (wide_initial.size() < 137) wide_initial += "01XZ";
  wide_initial.resize(137);
  while (wide_next.size() < 137) wide_next += "Z10X";
  wide_next.resize(137);
  sc_signal_rv<137> rv137("rv137", sc_lv<137>(wide_initial.c_str()));

  Payload payload_initial(3);
  sc_buffer<Payload> payload_buffer("payload_buffer", payload_initial);
  payload_buffer.write(Payload(9));

  auto trace = sc_create_vcd_trace_file("native_concrete_signals");
  trace->set_time_unit(1, SC_NS);
  sc_trace(trace, buffer, "buffer");
  sc_trace(trace, boolean, "boolean");
  sc_trace(trace, resolved, "resolved");
  sc_trace(trace, vector, "vector16");
  sc_trace(trace, resolved_multi, "resolved_multi");
  sc_trace(trace, vector_multi, "vector_multi");
  sc_trace(trace, rv137, "vector137");

  buffer.write(true);
  boolean.write(true);
  resolved.write(SC_LOGIC_X);
  vector.write(sc_lv<16>("01XZ"));
  rv137.write(sc_lv<137>(wide_next.c_str()));
  payload_buffer.write(Payload(9));
  if (buffer.read() || boolean.read() || resolved.read() != SC_LOGIC_Z || vector.read() != sc_lv<16>("0") ||
      rv137.read() != sc_lv<137>(wide_initial.c_str()))
    return 1;
  sc_start(1, SC_NS);
  if (observer.buffer_events.size() != 1 || observer.boolean_events.size() != 1 ||
      observer.resolved_events.size() != 1 || observer.vector_events.size() != 1)
    return 2;
  if (buffer.update_count != 1 || resolved.update_count != 1 || vector.update_count != 1 ||
      resolved.driver_count() != 1 || vector.driver_count() != 1)
    return 3;
  if (resolved_multi.read() != SC_LOGIC_X || vector_multi.read() != sc_lv<16>("0000000000000010") ||
      resolved_multi.driver_count() != 2 || vector_multi.driver_count() != 2 || resolved_multi.process_count() != 2 ||
      vector_multi.process_count() != 2 || rv137.read() != sc_lv<137>(wide_next.c_str()))
    return 4;
  if (payload_buffer.read().value != 9) return 5;

  buffer.write(true);
  boolean.write(false);
  resolved.write(SC_LOGIC_0);
  vector.write(sc_lv<16>("0000000000000010"));
  rv137.write(sc_lv<137>(wide_initial.c_str()));
  sc_start(1, SC_NS);
  if (observer.buffer_events.size() != 2 || observer.boolean_events.size() != 2 ||
      observer.resolved_events.size() != 2 || observer.vector_events.size() != 2)
    return 6;
  if (buffer.update_count != 2 || resolved.update_count != 2 || vector.update_count != 2) return 7;

  buffer.write(true);
  boolean.write(false);
  resolved.write(SC_LOGIC_0);
  vector.write(sc_lv<16>("0000000000000010"));
  sc_start(1, SC_NS);
  if (observer.buffer_events.size() != 3 || observer.boolean_events.size() != 2 ||
      observer.resolved_events.size() != 2 || observer.vector_events.size() != 2)
    return 8;
  if (buffer.update_count != 3 || resolved.update_count != 2 || vector.update_count != 2) return 9;
  if (!observer.buffer_events[0] || !observer.buffer_events[1] || !observer.buffer_events[2] ||
      !observer.boolean_events[0] || !observer.boolean_events[1] || !observer.resolved_events[0] ||
      !observer.resolved_events[1] || !observer.vector_events[0] || !observer.vector_events[1])
    return 10;
  if (&buffer.default_event() != &buffer.value_changed_event() ||
      &boolean.default_event() != &boolean.value_changed_event() ||
      &resolved.default_event() != &resolved.value_changed_event() ||
      &vector.default_event() != &vector.value_changed_event())
    return 11;

  sc_start(1, SC_NS);
  sc_close_vcd_trace_file(trace);

  emit_constructor("buffer", 0, buffer_default);
  emit_constructor("buffer", 1, buffer_named);
  emit_constructor("buffer", 2, buffer_initial);
  emit_constructor("resolved", 0, resolved_default);
  emit_constructor("resolved", 1, resolved_named);
  emit_constructor("resolved", 2, resolved_initial);
  emit_constructor("rv16", 0, rv_default);
  emit_constructor("rv16", 1, rv_named);
  emit_constructor("rv16", 2, rv_initial);
  emit_assignments("buffer", buffer_assign_value, buffer_assign_interface, buffer_assign_self);
  emit_assignments("resolved", resolved_assign_value, resolved_assign_interface, resolved_assign_self);
  emit_assignments("rv16", rv_assign_value, rv_assign_interface, rv_assign_self);
  std::cout << "CONCRETE_EVENTS " << observer.buffer_events.size() << '|' << observer.boolean_events.size() << '|'
            << observer.resolved_events.size() << '|' << observer.vector_events.size() << '|' << buffer.update_count
            << '|' << resolved.update_count << '|' << vector.update_count << '\n';
  const auto emit_deltas = [](const std::vector<sc_dt::uint64>& deltas)
  {
    for (std::size_t index = 0; index < deltas.size(); ++index)
    {
      if (index) std::cout << ',';
      std::cout << deltas[index];
    }
  };
  std::cout << "CONCRETE_DELTA ";
  emit_deltas(observer.buffer_deltas);
  std::cout << '|';
  emit_deltas(observer.boolean_deltas);
  std::cout << '|';
  emit_deltas(observer.resolved_deltas);
  std::cout << '|';
  emit_deltas(observer.vector_deltas);
  std::cout << '\n';
  std::cout << "CONCRETE_PAYLOAD " << payload_buffer.read().value << '\n';
  std::cout << "CONCRETE_DESTRUCTION " << destruction_before << '|' << destruction_after << '\n';
  std::cout << "CONCRETE_WIDE 137 " << rv137.read().to_string() << '\n';
  sc_stop();
  return 0;
}
