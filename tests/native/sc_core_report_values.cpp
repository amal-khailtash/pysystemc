/** @file sc_core_report_values.cpp
 * @brief Native report copy/getter/swap/message reference.
 * @details Borrowed handler report is copied before reporting returns; copies own strings/time.
 */
#include <iomanip>
#include <sstream>
#include <systemc>
#include <vector>
std::vector<sc_core::sc_report> owned;
std::vector<std::string> records;
/** @brief Copy callback report and emit exact composed message as hexadecimal. */
void handler(const sc_core::sc_report& report, const sc_core::sc_actions&)
{
  owned.push_back(report);
  const auto& copy = owned.back();
  if ((copy.valid() != (copy.get_process_name() != nullptr)) || copy.get_id() != -1 ||
      std::string(copy.get_file_name()) != "model.cpp" || copy.get_line_number() != 42 ||
      (copy.get_time().value() != 0 && copy.get_time().value() != 2000))
    throw std::runtime_error("getter");
  auto composed = sc_core::sc_report_compose_message(copy);
  if (composed != copy.what()) throw std::runtime_error("what");
  std::ostringstream out;
  out << "REPORT_VALUE_REFERENCE ";
  for (unsigned char ch : composed) out << std::hex << std::setw(2) << std::setfill('0') << unsigned(ch);
  records.push_back(out.str());
}
/** @brief Verify report copies outlive callbacks and swapping transfers values. */
int sc_main(int, char**)
{
  using namespace sc_core;
  auto previous = sc_report_handler::set_handler(handler);
  sc_report_handler::report(SC_INFO, "public/report", "first", "model.cpp", 42);
  sc_report_handler::report(SC_WARNING, "public/report", "second", "model.cpp", 42);
  sc_report_handler::set_handler(previous);
  if (owned.size() != 2 || std::string(owned[0].get_msg()) != "first" || std::string(owned[1].get_msg()) != "second" ||
      owned[0].get_severity() != SC_INFO || owned[1].get_severity() != SC_WARNING ||
      owned[0].get_verbosity() != SC_MEDIUM)
    return 2;
  owned[0].swap(owned[1]);
  if (std::string(owned[0].get_msg()) != "second" || std::string(owned[1].get_msg()) != "first") return 3;
  sc_report_handler::set_handler(handler);
  auto process = sc_spawn(
    []()
    {
      wait(2, SC_NS);
      sc_report_handler::report(SC_ERROR, "public/report", "thread", "model.cpp", 42);
    },
    "report_worker");
  sc_start(3, SC_NS);
  sc_report_handler::set_handler(previous);
  if (!process.terminated() || owned.size() != 3 || !owned[2].valid() ||
      std::string(owned[2].get_process_name()) != "report_worker" || owned[2].get_time().value() != 2000)
    return 4;
  sc_stop();
  for (auto& record : records) std::cout << record << "\n";
  std::cout << "REPORT_COPY_REFERENCE second first 0\n";
  return 0;
}
