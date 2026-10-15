/** @file Independent native reference for quantum synchronization and PEQ order. */
#include <tlm_utils/peq_with_get.h>
#include <tlm_utils/tlm_quantumkeeper.h>

#include <iostream>
#include <stdexcept>
#include <systemc>
#include <tlm>

/** @brief Enforce reference assertions even with native NDEBUG builds. */
void verify(bool condition)
{
  if (!condition) throw std::runtime_error("native utility reference assertion failed");
}

/** @brief Own queue and payloads for the entire simulation. */
struct UtilityReference : sc_core::sc_module
{
  tlm_utils::peq_with_get<tlm::tlm_generic_payload> queue;
  tlm_utils::tlm_quantumkeeper keeper;
  tlm::tlm_generic_payload first, second;
  SC_HAS_PROCESS(UtilityReference);

  /** @brief Register independent native processes; no Python callbacks or GIL. */
  explicit UtilityReference(sc_core::sc_module_name name) : sc_module(name), queue("queue")
  {
    keeper.set_global_quantum(sc_core::sc_time(5, sc_core::SC_NS));
    keeper.reset();
    first.set_address(17);
    second.set_address(23);
    queue.notify(first, sc_core::sc_time(2, sc_core::SC_NS));
    queue.notify(second, sc_core::sc_time(4, sc_core::SC_NS));
    SC_THREAD(quantum);
    SC_THREAD(receive);
  }

  /** @brief Check temporal decoupling before and after both native sync APIs. */
  void quantum()
  {
    keeper.inc(sc_core::sc_time(3, sc_core::SC_NS));
    verify(!keeper.need_sync());
    verify(keeper.get_current_time() == sc_core::sc_time(3, sc_core::SC_NS));
    keeper.inc(sc_core::sc_time(2, sc_core::SC_NS));
    verify(keeper.need_sync());
    keeper.sync();
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(5, sc_core::SC_NS));
    verify(keeper.get_local_time() == sc_core::SC_ZERO_TIME);
    keeper.set_and_sync(sc_core::sc_time(5, sc_core::SC_NS));
    std::cout << "UTILITY quantum " << sc_core::sc_time_stamp() << '\n';
  }

  /** @brief Drain borrowed transactions and verify cancellation without delivery. */
  void receive()
  {
    for (int index = 0; index < 2; ++index)
    {
      auto* expected = index == 0 ? &first : &second;
      wait(queue.get_event());
      auto* payload = queue.get_next_transaction();
      verify(payload == expected);
      std::cout << "UTILITY " << payload->get_address() << ' ' << sc_core::sc_time_stamp() << '\n';
      auto* pending = queue.get_next_transaction();
      if (pending != nullptr) throw std::runtime_error("unexpected due payload");
    }
    queue.notify(first, sc_core::sc_time(1, sc_core::SC_NS));
    queue.cancel_all();
    wait(2, sc_core::SC_NS);
    verify(queue.get_next_transaction() == nullptr);
  }
};

/** @brief Run the reference kernel once; model owns resources until completion. */
int sc_main(int, char**)
{
  UtilityReference model("top");
  sc_core::sc_start(11, sc_core::SC_NS);
  return 0;
}
