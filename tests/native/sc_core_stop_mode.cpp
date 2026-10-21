/** @file sc_core_stop_mode.cpp
 * @brief Native stop-mode setters/getters and simulator status reference.
 * @details Static kernel configuration, no ownership transfer or callbacks.
 */
#include <systemc>
/** @brief Verify stop-mode mutation and after-start report rejection. */
int sc_main(int, char**)
{
  using namespace sc_core;
  if (sc_get_stop_mode() != SC_STOP_FINISH_DELTA) return 2;
  sc_set_stop_mode(SC_STOP_IMMEDIATE);
  if (sc_get_stop_mode() != SC_STOP_IMMEDIATE) return 3;
  sc_set_stop_mode(SC_STOP_FINISH_DELTA);
  auto before = sc_get_simulator_status();
  sc_start(1, SC_NS, SC_RUN_TO_TIME);
  try
  {
    sc_set_stop_mode(SC_STOP_IMMEDIATE);
    return 4;
  }
  catch (const sc_report&)
  {
  }
  if (sc_get_stop_mode() != SC_STOP_FINISH_DELTA) return 5;
  auto paused = sc_get_simulator_status();
  sc_stop();
  std::cout << "STOP_MODE_REFERENCE " << before << " " << paused << " " << sc_get_simulator_status() << " "
            << int(sc_get_stop_mode()) << "\n";
  return 0;
}
