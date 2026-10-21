/** @file sc_core_time_configuration.cpp
 * @brief Native time configuration/reference and report rejection semantics.
 * @details Time values are copied; maximum time is library-owned. No callbacks.
 */
#include <systemc>
int errors = 0;
/** @brief Require native report exception without suppressing subsequent recovery. */
template <class F>
void rejected(F action)
{
  try
  {
    action();
    throw std::runtime_error("invalid accepted");
  }
  catch (const sc_core::sc_report&)
  {
    ++errors;
  }
}
/** @brief Compare valid, duplicate and running-time configuration behavior. */
int sc_main(int, char**)
{
  using namespace sc_core;
  rejected([] { sc_set_time_resolution(-1, SC_PS); });
  rejected([] { sc_set_time_resolution(3, SC_PS); });
  sc_set_time_resolution(10, SC_PS);
  rejected([] { sc_set_default_time_unit(-1, SC_NS); });
  rejected([] { sc_set_default_time_unit(3, SC_NS); });
  sc_set_default_time_unit(10, SC_NS);
  rejected([] { sc_set_time_resolution(10, SC_PS); });
  rejected([] { sc_set_default_time_unit(10, SC_NS); });
  auto limit = sc_max_time();
  sc_start(25, SC_NS, SC_RUN_TO_TIME);
  rejected([] { sc_set_time_resolution(10, SC_PS); });
  rejected([] { sc_set_default_time_unit(10, SC_NS); });
  sc_stop();
  auto default_unit = sc_get_default_time_unit();
  auto simulation_time = sc_simulation_time();
  std::cout << "TIME_CONFIGURATION_REFERENCE " << sc_get_time_resolution().value() << " " << default_unit.value() << " "
            << limit.value() << " " << sc_time_stamp().value() << " " << simulation_time << " " << errors << "\n";
  return 0;
}
