/** @file sc_core_copy_only_values.cpp
 * @brief Native reference for copied inputs through eight write/put interfaces.
 */
#include "sc_core_copy_only_values.h"

#include <iostream>
#include <memory>
#include <systemc>
#include <tlm>
#include <vector>

using value_type = copy_only::transaction;

/** @brief Retain owning copies while interfaces borrow this callback owner.
 * @details Callbacks run synchronously on the simulation thread and do not wait.
 * Inputs and delays are borrowed only during entry; samples own independent
 * allocations. Copy-constructor exceptions propagate without source mutation.
 */
struct sink : tlm::tlm_analysis_if<value_type>,
              tlm::tlm_delayed_analysis_if<value_type>,
              tlm::tlm_put_if<value_type>,
              sc_core::sc_signal_write_if<value_type>,
              sc_core::sc_fifo_blocking_out_if<value_type>
{
  std::vector<std::unique_ptr<value_type>> samples;
  sc_dt::uint64 delay = 0;
  sc_core::sc_event ready;

  /** @brief Copy an input into owned storage shared by the ordinary write views. */
  void write(const value_type& value) override
  {
    samples.emplace_back(new value_type(value));
  }
  /** @brief Copy an input and snapshot the borrowed delay synchronously. */
  void write(const value_type& value, const sc_core::sc_time& time) override
  {
    write(value);
    delay = time.value();
  }
  /** @brief Copy an input through the native blocking put interface. */
  void put(const value_type& value) override
  {
    write(value);
  }
  /** @brief Accept a copied nonblocking input without waiting. */
  bool nb_put(const value_type& value) override
  {
    write(value);
    return true;
  }
  /** @brief Report available capacity without waiting or transferring ownership. */
  bool nb_can_put(tlm::tlm_tag<value_type>* = nullptr) const override
  {
    return true;
  }
  /** @brief Borrow the owner's event through native nonblocking readiness. */
  const sc_core::sc_event& ok_to_put(tlm::tlm_tag<value_type>* = nullptr) const override
  {
    return ready;
  }
  /** @brief Report the native one-writer policy. */
  sc_core::sc_writer_policy get_writer_policy() const override
  {
    return sc_core::SC_ONE_WRITER;
  }
};

/** @brief Exercise independent storage and survival after caller destruction. */
template <class Operation>
void exercise(sink& target, const char* label, int index, Operation operation)
{
  target.delay = 0;
  {
    value_type input(73 + index);
    operation(input);
    if (target.samples.back().get() == &input) throw std::runtime_error("input alias");
    target.samples.back()->value += 100;
    if (input.value != 73 + index) throw std::runtime_error("source mutation");
  }
  if (value_type::live != static_cast<int>(target.samples.size())) throw std::runtime_error("value lifetime");
  std::cout << "COPY_ONLY_VALUE " << label << ' ' << target.samples.back()->value << ' ' << target.delay << '\n';
}

/** @brief Compare native copy-only callback values and rejected construction. */
int sc_main(int, char**)
{
  sink target;
  exercise(target, "analysis", 0,
           [&](const auto& value) { static_cast<tlm::tlm_analysis_if<value_type>&>(target).write(value); });
  exercise(target, "write", 1,
           [&](const auto& value) { static_cast<tlm::tlm_write_if<value_type>&>(target).write(value); });
  exercise(target, "delayed_analysis", 2,
           [&](const auto& value) {
             static_cast<tlm::tlm_delayed_analysis_if<value_type>&>(target).write(value,
                                                                                  sc_core::sc_time(2, sc_core::SC_NS));
           });
  exercise(
    target, "delayed_write", 3,
    [&](const auto& value)
    { static_cast<tlm::tlm_delayed_write_if<value_type>&>(target).write(value, sc_core::sc_time(2, sc_core::SC_NS)); });
  exercise(target, "blocking_put", 4,
           [&](const auto& value) { static_cast<tlm::tlm_blocking_put_if<value_type>&>(target).put(value); });
  exercise(target, "nonblocking_put", 5,
           [&](const auto& value)
           {
             if (!static_cast<tlm::tlm_nonblocking_put_if<value_type>&>(target).nb_put(value))
               throw std::runtime_error("put rejected");
           });
  exercise(target, "signal_write", 6,
           [&](const auto& value) { static_cast<sc_core::sc_signal_write_if<value_type>&>(target).write(value); });
  exercise(target, "fifo_write", 7,
           [&](const auto& value) { static_cast<sc_core::sc_fifo_blocking_out_if<value_type>&>(target).write(value); });

  {
    value_type input(999);
    const auto count = target.samples.size();
    value_type::reject_copy = true;
    bool rejected = false;
    try
    {
      target.write(input);
    }
    catch (const std::runtime_error&)
    {
      rejected = true;
    }
    value_type::reject_copy = false;
    if (!rejected || input.value != 999 || target.samples.size() != count) return 2;
  }
  target.samples.clear();
  if (value_type::live != 0) return 3;
  std::cout << "COPY_ONLY_FAILURE 1 999 0\n";
  return 0;
}
