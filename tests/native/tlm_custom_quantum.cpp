/** @file Native reference for overridden local quantum synchronization. */
#include <tlm_utils/tlm_quantumkeeper.h>

#include <iostream>
#include <stdexcept>
#include <systemc>

/** @brief Enforce reference checks independently of NDEBUG; no Python or GIL. */
void verify(bool value)
{
  if (!value) throw std::runtime_error("custom quantum assertion failed");
}
/** @brief Native keeper overrides the same protected hook as the Python adapter. */
struct Keeper : tlm_utils::tlm_quantumkeeper
{
  /** @brief Return an owned three-nanosecond quantum by value. */
  sc_core::sc_time compute_local_quantum() override
  {
    return sc_core::sc_time(3, sc_core::SC_NS);
  }
};
/** @brief Own the keeper throughout native process suspension. */
struct Model : sc_core::sc_module
{
  Keeper keeper;
  SC_HAS_PROCESS(Model);
  /** @brief Initialize the quantum and register a native pthread process. */
  explicit Model(sc_core::sc_module_name name) : sc_module(name)
  {
    keeper.set_global_quantum(sc_core::sc_time(10, sc_core::SC_NS));
    keeper.reset();
    SC_THREAD(run);
  }
  /** @brief Check exact synchronization thresholds and both wait paths. */
  void run()
  {
    keeper.inc(sc_core::sc_time(2, sc_core::SC_NS));
    verify(!keeper.need_sync());
    keeper.inc(sc_core::sc_time(1, sc_core::SC_NS));
    verify(keeper.need_sync());
    keeper.sync();
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(3, sc_core::SC_NS));
    verify(keeper.get_local_time() == sc_core::SC_ZERO_TIME);
    std::cout << "CUSTOM_QUANTUM " << sc_core::sc_time_stamp() << '\n';
    keeper.set_and_sync(sc_core::sc_time(2, sc_core::SC_NS));
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(3, sc_core::SC_NS));
    keeper.set_and_sync(sc_core::sc_time(3, sc_core::SC_NS));
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(6, sc_core::SC_NS));
    std::cout << "CUSTOM_QUANTUM " << sc_core::sc_time_stamp() << '\n';
  }
};
/** @brief Execute one bounded native simulation; resources outlive all waits. */
int sc_main(int, char**)
{
  Model model("top");
  sc_core::sc_start(7, sc_core::SC_NS);
  return 0;
}
