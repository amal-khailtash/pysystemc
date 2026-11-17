/** @file sc_core_report_lifetime_edges.cpp
 * @brief Native reference for report assignment and timestamp lifetimes.
 */
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <systemc>
#include <vector>

namespace
{
std::vector<sc_core::sc_report> reports;

/** @brief Copy the callback-borrowed report before its native lifetime ends.
 * @details The vector owns copied reports and their timestamp allocations after
 * callback return. This runs synchronously on the simulation thread and does
 * not retain Python references or interact with the GIL.
 */
void capture_report(const sc_core::sc_report& report, const sc_core::sc_actions&)
{
  reports.push_back(report);
}

/** @brief Return the address of the timestamp borrowed from a live report.
 * @details The result is an integer snapshot only. Callers must not dereference
 * a prior timestamp reference after assignment or report destruction.
 */
std::uintptr_t timestamp_address(const sc_core::sc_report& report)
{
  return reinterpret_cast<std::uintptr_t>(&report.get_time());
}

/** @brief Print copied report values in the same format as the Python model. */
void print_report(const sc_core::sc_report& report)
{
  const auto composed = std::string(report.what());
  std::ostringstream encoded;
  for (unsigned char character : composed)
    encoded << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(character);
  std::cout << "REPORT_TIME_CLASS_ASSIGN 1 " << report.get_id() << ' ' << static_cast<int>(report.get_severity()) << ' '
            << report.get_msg_type() << ' ' << report.get_msg() << ' ' << report.get_file_name() << ' '
            << report.get_line_number() << ' ' << report.get_time().value() << ' ' << report.get_verbosity() << ' '
            << encoded.str() << '\n';
}
}  // namespace

/** @brief Compare copied report metadata, assignment behavior, and timestamp identity. */
int sc_main(int, char**)
{
  using namespace sc_core;
  const auto previous_handler = sc_report_handler::set_handler(capture_report);
  auto destination_process = sc_spawn(
    []()
    {
      wait(1, SC_NS);
      sc_report_handler::report(SC_INFO, "assignment/destination", "old", "dest.cpp", 11);
    },
    "destination_worker");
  auto source_process = sc_spawn(
    []()
    {
      wait(2, SC_NS);
      sc_report_handler::report(SC_WARNING, "assignment/source", "copied", "source.cpp", 73);
    },
    "source_worker");
  auto alias_process = sc_spawn(
    []()
    {
      wait(3, SC_NS);
      sc_report_handler::report(SC_ERROR, "assignment/alias-target", "alias", "alias.cpp", 97);
    },
    "alias_worker");
  sc_start(4, SC_NS);
  sc_report_handler::set_handler(previous_handler);
  if (!destination_process.terminated() || !source_process.terminated() || !alias_process.terminated() ||
      reports.size() != 3)
    return 2;

  auto& destination = reports[0];
  auto& source = reports[1];
  auto& alias_target = reports[2];
  const auto destination_address = timestamp_address(destination);
  const auto source_address = timestamp_address(source);
  const auto alias_address = timestamp_address(alias_target);
  if (destination.get_time().value() != 1000 || source.get_time().value() != 2000 ||
      alias_target.get_time().value() != 3000 || destination_address != timestamp_address(destination) ||
      destination_address == source_address || destination_address == alias_address || source_address == alias_address)
    return 3;
  std::cout << "REPORT_TIME_REFERENCE 1 1 1\n";

  // C++ keeps the native reference return and checks it aliases destination.
  sc_report& assigned = destination.operator=(source);
  const auto assigned_address = timestamp_address(destination);
  if (&assigned != &destination || assigned_address == destination_address || assigned_address == source_address ||
      destination.get_time().value() != source.get_time().value() ||
      std::string(destination.get_process_name()) != std::string(source.get_process_name()))
    return 4;
  std::cout << "REPORT_TIME_ASSIGN 1 1 " << destination.get_time().value() << '\n';
  print_report(destination);

  const auto self_old_address = timestamp_address(destination);
  const auto self_old_time = destination.get_time().value();
  sc_report& self_result = destination.operator=(destination);
  const auto self_new_address = timestamp_address(destination);
  if (&self_result != &destination || self_new_address == self_old_address ||
      destination.get_time().value() != self_old_time)
    return 5;
  std::cout << "REPORT_TIME_SELF 1 1 " << destination.get_time().value() << '\n';

  const auto alias_old_address = timestamp_address(alias_target);
  const auto alias_old_time = alias_target.get_time().value();
  sc_report& alias_self_result = alias_target.operator=(alias_target);
  if (&alias_self_result != &alias_target || timestamp_address(alias_target) == alias_old_address ||
      alias_target.get_time().value() != alias_old_time)
    return 6;

  sc_report& class_assigned = destination.operator=(alias_target);
  if (&class_assigned != &destination || timestamp_address(destination) == self_new_address ||
      timestamp_address(destination) == alias_address || destination.get_time().value() != 3000 ||
      std::string(destination.get_msg_type()) != std::string(alias_target.get_msg_type()))
    return 7;

  const auto left_address = timestamp_address(destination);
  const auto right_address = timestamp_address(source);
  const auto left_time = destination.get_time().value();
  const auto right_time = source.get_time().value();
  destination.swap(source);
  if (timestamp_address(destination) != right_address || timestamp_address(source) != left_address ||
      destination.get_time().value() != right_time || source.get_time().value() != left_time)
    return 8;
  std::cout << "REPORT_TIME_SWAP 1 " << destination.get_time().value() << ' ' << source.get_time().value() << " 1\n";
  sc_stop();
  return 0;
}
