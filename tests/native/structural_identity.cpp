/** @file structural_identity.cpp
 * @brief Independent native inherited FIFO/TLM interface identity reference.
 * @details Channels own their values and virtual interface bases. No Python
 * callbacks, GIL operations or borrowed storage outlive this simulation.
 */
#include <deque>
#include <iostream>
#include <stdexcept>
#include <systemc>
#include <tlm>

/** @brief Native combined FIFO input with immediate sample storage. */
struct fifo_input : sc_core::sc_module, sc_core::sc_fifo_in_if<int>
{
  std::deque<int> values{41, 43};
  sc_core::sc_event changed;
  explicit fifo_input(sc_core::sc_module_name name) : sc_module(name)
  {
  }
  /** @brief Consume one sample; empty reads fail without waiting. */
  int read() override
  {
    if (values.empty()) throw std::runtime_error("empty");
    auto v = values.front();
    values.pop_front();
    return v;
  }
  /** @brief Assign the consumed sample to caller-owned storage. */
  void read(int& value) override
  {
    value = read();
  }
  /** @brief Preserve caller output when no sample exists. */
  bool nb_read(int& value) override
  {
    if (values.empty()) return false;
    value = read();
    return true;
  }
  /** @brief Count stored samples. */
  int num_available() const override
  {
    return values.size();
  }
  /** @brief Borrow the stable reader event. */
  const sc_core::sc_event& data_written_event() const override
  {
    return changed;
  }
};

/** @brief Native combined get/peek interface with shared sample storage. */
struct get_peek : sc_core::sc_module, tlm::tlm_get_peek_if<int>
{
  std::deque<int> values{41, 43};
  sc_core::sc_event ready;
  explicit get_peek(sc_core::sc_module_name name) : sc_module(name)
  {
  }
  /** @brief Consume the oldest value. */
  int get(tlm::tlm_tag<int>* = nullptr) override
  {
    auto v = values.front();
    values.pop_front();
    return v;
  }
  /** @brief Assign a consumed value to caller-owned storage. */
  void get(int& value) override
  {
    value = get();
  }
  /** @brief Inspect the oldest value without consuming it. */
  int peek(tlm::tlm_tag<int>* = nullptr) const override
  {
    return values.front();
  }
  /** @brief Copy the oldest value into caller storage. */
  void peek(int& value) const override
  {
    value = peek();
  }
  /** @brief Preserve output on empty get. */
  bool nb_get(int& value) override
  {
    if (values.empty()) return false;
    value = get();
    return true;
  }
  /** @brief Preserve output on empty peek. */
  bool nb_peek(int& value) const override
  {
    if (values.empty()) return false;
    value = peek();
    return true;
  }
  /** @brief Query availability without changing state. */
  bool nb_can_get(tlm::tlm_tag<int>* = nullptr) const override
  {
    return !values.empty();
  }
  /** @brief Query peek availability without changing state. */
  bool nb_can_peek(tlm::tlm_tag<int>* = nullptr) const override
  {
    return !values.empty();
  }
  /** @brief Borrow the stable native readiness event. */
  const sc_core::sc_event& ok_to_get(tlm::tlm_tag<int>* = nullptr) const override
  {
    return ready;
  }
  /** @brief Borrow the stable native readiness event. */
  const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<int>* = nullptr) const override
  {
    return ready;
  }
};

/** @brief Verify virtual base identity and shared state before simulation. */
int sc_main(int, char**)
{
  fifo_input fifo("fifo");
  get_peek tlm_channel("tlm");
  sc_core::sc_fifo_blocking_in_if<int>& blocking = fifo;
  sc_core::sc_fifo_nonblocking_in_if<int>& nonblocking = fifo;
  if (static_cast<sc_core::sc_interface*>(&blocking) != static_cast<sc_core::sc_interface*>(&nonblocking))
    throw std::runtime_error("FIFO identity");
  tlm::tlm_blocking_get_if<int>& get = tlm_channel;
  tlm::tlm_nonblocking_peek_if<int>& peek = tlm_channel;
  if (static_cast<sc_core::sc_interface*>(&get) != static_cast<sc_core::sc_interface*>(&peek))
    throw std::runtime_error("TLM identity");
  auto fv = blocking.read();
  auto tv = get.get();
  int next = 0;
  if (!peek.nb_peek(next)) throw std::runtime_error("peek failed");
  std::cout << "IDENTITY_REFERENCE " << fv << " " << fifo.num_available() << " " << tv << " " << next << " "
            << sc_core::sc_time_stamp() << "\n";
  sc_core::sc_start(1, sc_core::SC_NS);
  sc_core::sc_stop();
  return 0;
}
