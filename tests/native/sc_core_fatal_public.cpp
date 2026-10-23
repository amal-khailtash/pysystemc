/** @file sc_core_fatal_public.cpp
 * @brief Native assertion and abort reference for isolated subprocess tests.
 */
#include <sys/resource.h>

#include <iostream>
#include <string>
#include <systemc>
/** @brief Verify selected fatal actions or enter native abort without core dumps. */
int sc_main(int argc, char** argv)
{
  rlimit limits{0, 0};
  setrlimit(RLIMIT_CORE, &limits);
  const std::string mode = argc > 1 ? argv[1] : "throw";
  if (mode == "throw")
  {
    sc_core::sc_report_handler::set_actions(sc_core::SC_FATAL, sc_core::SC_THROW);
    try
    {
      sc_core::sc_assertion_failed("sample assertion", "portable_probe.cpp", 17);
    }
    catch (const sc_core::sc_report& report)
    {
      sc_assert(report.get_severity() == sc_core::SC_FATAL && report.get_line_number() == 17);
      std::cout << "FATAL_THROW|" << report.get_id() << '|' << report.get_msg_type() << '|' << report.get_file_name()
                << '|' << report.get_line_number() << '\n';
      return 0;
    }
    return 1;
  }
  std::cout << "FATAL_ENTER " << mode << std::endl;
  if (mode == "abort")
    sc_core::sc_abort();
  else
    sc_core::sc_assertion_failed("sample assertion", "portable_probe.cpp", 17);
  return 2;
}
