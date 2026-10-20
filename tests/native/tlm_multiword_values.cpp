/** @file tlm_multiword_values.cpp
 * @brief Independent native FIFO reference for multiword get/peek overloads.
 * @details FIFO owns value copies. Interface views borrow its lifetime in sc_main.
 * No Python callbacks, GIL operations, or references escape the reference process.
 */
#include <iostream>
#include <stdexcept>
#include <systemc>
#include <tlm>

/** @brief Check native value/output overloads and failed nonblocking output. */
template <class T>
void check(const char* name)
{
  tlm::tlm_fifo<T> fifo("fifo", 2);
  T expected = 17;
  if constexpr (std::is_same_v<T, sc_dt::sc_biguint<137>>) expected[136] = true;
  tlm::tlm_get_if<T>& get = fifo;
  tlm::tlm_peek_if<T>& peek = fifo;
  tlm::tlm_nonblocking_get_if<T>& get_base = get;
  tlm::tlm_nonblocking_peek_if<T>& peek_base = peek;
  if (&get.ok_to_get() != &get_base.ok_to_get() || &peek.ok_to_peek() != &peek_base.ok_to_peek())
    throw std::runtime_error("inherited readiness event identity mismatch");
  T output = 71;
  if (get.nb_get(output) || output != T(71)) throw std::runtime_error("empty get modified output");
  output = 73;
  if (peek.nb_can_peek() || get.nb_can_get() || peek.nb_peek(output) || output != T(73))
    throw std::runtime_error("empty readiness or peek output mismatch");
  fifo.put(expected);
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  if (!get.nb_can_get() || !peek.nb_can_peek()) throw std::runtime_error("ready sample not reported");
  if (!peek.nb_peek(output) || output != expected) throw std::runtime_error("nonblocking peek mismatch");
  output = 71;
  if (!get.nb_get(output) || output != expected) throw std::runtime_error("nonblocking get mismatch");
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  output = 73;
  if (get.nb_can_get() || peek.nb_can_peek() || peek.nb_peek(output) || output != T(73))
    throw std::runtime_error("consumed sample remains ready");
  fifo.put(expected);
  fifo.put(expected);
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  if (peek.peek() != expected) throw std::runtime_error("peek value mismatch");
  peek.peek(output);
  if (output != expected) throw std::runtime_error("peek output mismatch");
  if (get.get() != expected) throw std::runtime_error("get value mismatch");
  get.get(output);
  if (output != expected) throw std::runtime_error("get output mismatch");
  std::cout << "TLM_VALUE " << name << " get 1 1\n";
  std::cout << "TLM_VALUE " << name << " peek 1 1\n";
  std::cout << "TLM_NB " << name << " get 0 1 1\n";
  std::cout << "TLM_NB " << name << " peek 0 1 1\n";
  std::cout << "TLM_EVENT " << name << " get 1\n";
  std::cout << "TLM_EVENT " << name << " peek 1\n";
}

/** @brief Select one specialization per independent native kernel process. */
int sc_main(int argc, char** argv)
{
  if (argc != 2) return 2;
  std::string type = argv[1];
  if (type == "long_long")
    check<long long>(argv[1]);
  else if (type == "unsigned_long_long")
    check<unsigned long long>(argv[1]);
  else if (type == "long_double")
    check<long double>(argv[1]);
  else if (type == "biguint137")
    check<sc_dt::sc_biguint<137>>(argv[1]);
  else
    return 2;
  return 0;
}
