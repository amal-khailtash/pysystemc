/** @file sc_core_entry_error_lifetime.h
 *  @brief Native entry failures for Python report-handler cleanup checks.
 *  @details The native model emits an ordinary report, throws a native report,
 *  or throws an ordinary exception through the entry wrapper.
 */
#include <stdexcept>
#include <string>
#include <systemc>

/** @brief Enter a selected native failure through SystemC's sc_main wrapper.
 *  @param argc Native argument count.
 *  @param argv Native argument array; entry wrapper owns its copied values.
 *  @return Zero if the selected failure unexpectedly returns.
 */
int sc_main(int argc, char** argv)
{
  using namespace sc_core;
  if (argc != 2 || !argv || !argv[1]) return 2;
  const std::string scenario(argv[1]);
  if (scenario == "runtime") throw std::runtime_error("native entry runtime failure");
  if (scenario == "direct")
  {
    sc_report_handler::set_actions("entry/lifetime", SC_WARNING, SC_DO_NOTHING);
    SC_REPORT_WARNING("entry/lifetime", "direct callback report");
    return 0;
  }
  if (scenario == "deprecation")
  {
    SC_REPORT_INFO(SC_ID_IEEE_1666_DEPRECATION_, "entry deprecation counter marker");
    return 0;
  }
  if (scenario == "report")
  {
    const auto python_handler = sc_report_handler::get_handler();
    sc_report_handler::set_actions("entry/lifetime", SC_ERROR, SC_THROW);
    // Use SystemC's native handler to create a real sc_report for the entry
    // wrapper, then restore Python dispatch before rethrowing into its catch.
    sc_report_handler::set_handler(sc_report_handler::default_handler);
    try
    {
      SC_REPORT_ERROR("entry/lifetime", "native entry report failure");
    }
    catch (const sc_report&)
    {
      sc_report_handler::set_handler(python_handler);
      throw;
    }
  }
  return 3;
}
