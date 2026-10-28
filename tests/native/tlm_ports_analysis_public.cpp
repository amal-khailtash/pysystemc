/** @file tlm_ports_analysis_public.cpp
 *  @brief Native reference for arbitrary-width TLM1 ports and analysis APIs.
 *  @details Event delivery uses native process sensitivity. Analysis
 *  subscribers copy transactions and analysis FIFOs retain values by value.
 */
#include <tlm_core/tlm_1/tlm_analysis/tlm_analysis_fifo.h>

#include <iostream>

#include "tlm_ports_analysis_types.h"

// The bundled analysis FIFO calls a dependent-base nb_put without `this->`,
// which GCC and Cling reject. Specialize the native overloads for the two
// concrete payloads before either FIFO type is instantiated by the probe.
template <>
inline void tlm::tlm_analysis_fifo<tlm_ports_analysis_probe::Sample>::write(
  const tlm_ports_analysis_probe::Sample& sample)
{
  this->nb_put(sample);
}

template <>
inline void tlm::tlm_analysis_fifo<tlm_ports_analysis_probe::Sample>::write(
  const tlm::tlm_analysis_triple<tlm_ports_analysis_probe::Sample>& triple)
{
  this->nb_put(triple.transaction);
}

template <>
inline void tlm::tlm_analysis_fifo<int>::write(const int& value)
{
  this->nb_put(value);
}

template <>
inline void tlm::tlm_analysis_fifo<int>::write(const tlm::tlm_analysis_triple<int>& triple)
{
  this->nb_put(triple.transaction);
}

#include "tlm_ports_analysis_probe.h"

int sc_main(int, char**)
{
  using namespace tlm_ports_analysis_probe;
  {
    Top top("top");
    tlm::tlm_analysis_fifo<int> int_fifo;
    top.get_port.bind(top.get_store);
    top.peek_port.bind(top.peek_store);
    top.put_port.bind(top.put_store);

    auto* finder = make_get_finder(top.get_port);
    verify(&finder->find_event() == &top.get_store.ready, "event finder resolved the wrong interface event");
    delete finder;

    top.analysis_default.bind(top.sink_a);
    top.analysis_default(top.sink_b);
    top.analysis_named.bind(top.sink_c);
    const Sample first(1, 1001);
    const Sample second(2, 2002);
    const Sample third(3, 3003);
    const Sample named(9, 9009);
    top.analysis_default.write(first);
    top.analysis_default.write(second);
    verify(top.analysis_default.unbind(top.sink_b), "analysis port failed to unbind subscriber");
    verify(!top.analysis_default.unbind(top.sink_b), "analysis port unbound a missing subscriber");
    top.analysis_default.write(third);
    top.analysis_named.write(named);

    const Sample fifo_default_value(40, 4040);
    const Sample fifo_first(50, 5050);
    const Sample fifo_second(60, 6060);
    top.fifo_default.write(fifo_default_value);
    top.fifo_named.write(fifo_first);
    int_fifo.write(73);
    const tlm::tlm_analysis_triple<int> int_triple(84);
    int_fifo.write(int_triple);
    Top::triple_type blank_triple;
    blank_triple.transaction = Sample(39, 3939);
    verify(blank_triple.transaction.sequence == 39, "default analysis triple construction mismatch");
    Top::triple_type triple(fifo_second);
    triple.start_time = sc_core::sc_time(1, sc_core::SC_NS);
    triple.end_time = sc_core::sc_time(4, sc_core::SC_NS);
    Top::triple_type triple_copy(triple);
    const Sample by_value = triple_to_value(triple);
    const Sample& by_reference = triple_to_const_reference(triple_copy);
    verify(by_value.sequence == 60 && by_reference.sequence == 60, "analysis triple conversion mismatch");
    top.fifo_named.write(triple);

    sc_core::sc_start(4, sc_core::SC_NS);
    verify(top.get_seen && top.peek_seen && top.put_seen, "nonblocking port event sensitivity did not run");
    verify(!top.get_store.available && top.peek_store.available && !top.put_store.available,
           "nonblocking get/peek/put consumption semantics mismatch");
    verify(top.port_events == std::vector<std::string>{"peek|1|20|202|1", "get|1|10|101|1", "put|3|30|303|1"},
           "nonblocking port event order or simulation time mismatch");
    verify(top.sink_a.received.size() == 3 && top.sink_a.received[0].sequence == 1 &&
             top.sink_a.received[1].sequence == 2 && top.sink_a.received[2].sequence == 3,
           "analysis subscriber A delivery order mismatch");
    verify(
      top.sink_b.received.size() == 2 && top.sink_b.received[0].sequence == 1 && top.sink_b.received[1].sequence == 2,
      "analysis subscriber B delivery order mismatch");
    verify(top.sink_c.received.size() == 1 && top.sink_c.received[0].sequence == 9,
           "named analysis subscriber delivery mismatch");

    Sample default_read;
    Sample named_read_first;
    Sample named_read_second;
    int int_read_first = 0;
    int int_read_second = 0;
    verify(top.fifo_default.nb_get(default_read), "default analysis FIFO was empty");
    verify(top.fifo_named.nb_get(named_read_first), "named analysis FIFO first read failed");
    verify(top.fifo_named.nb_get(named_read_second), "named analysis FIFO triple read failed");
    verify(int_fifo.nb_get(int_read_first) && int_fifo.nb_get(int_read_second), "int analysis FIFO read failed");
    verify(default_read.sequence == 40 && named_read_first.sequence == 50 && named_read_second.sequence == 60,
           "analysis FIFO delivery order mismatch");

    for (const auto& event : top.port_events) std::cout << "PORT_ANALYSIS_EVENT " << event << '\n';
    std::cout << "ANALYSIS_PORT " << top.sink_a.received.size() << ' ' << top.sink_a.received[0].sequence << ' '
              << top.sink_a.received[1].sequence << ' ' << top.sink_b.received.size() << ' '
              << top.sink_b.received[0].sequence << ' ' << top.sink_c.received.size() << ' '
              << top.sink_c.received[0].sequence << '\n';
    std::cout << "ANALYSIS_FIFO " << default_read.sequence << ' ' << default_read.bits.to_uint64() << ' '
              << named_read_first.sequence << ' ' << named_read_second.sequence << ' '
              << named_read_second.bits.to_uint64() << '\n';
    std::cout << "ANALYSIS_FIFO_INT " << int_read_first << ' ' << int_read_second << '\n';
    std::cout << "ANALYSIS_TRIPLE " << triple.start_time << ' ' << triple.transaction.sequence << ' ' << triple.end_time
              << ' ' << triple_copy.transaction.sequence << ' ' << by_value.sequence << ' ' << by_reference.sequence
              << '\n';
  }
  verify(Top::destroyed == 1, "port/analysis owner lifetime mismatch");
  std::cout << "PORT_ANALYSIS_LIFETIME 1\n";
  std::cout << "TLM_PORTS_ANALYSIS_PUBLIC_OK\n";
  sc_core::sc_stop();
  return 0;
}
