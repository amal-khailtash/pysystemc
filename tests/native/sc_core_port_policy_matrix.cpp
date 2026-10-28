/** @file sc_core_port_policy_matrix.cpp
 * @brief Independent native reference for every port policy binding boundary.
 * @details Each case owns its host, ports and signals through elaboration. Ports
 * borrow signals. Report callbacks run synchronously with no Python GIL use.
 */
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

namespace port_policy_reference
{
/** @brief Provide legal public construction of a native module host. */
struct Host : sc_core::sc_module
{
  /** @brief Borrow the active module-name token during construction. */
  explicit Host(sc_core::sc_module_name name) : sc_module(name)
  {
  }
};
/** @brief Own one port and its borrowed signal dependencies in destruction order. */
struct Case
{
  std::string label;
  int maximum;
  int count;
  std::unique_ptr<Host> top;
  std::vector<std::unique_ptr<sc_core::sc_signal<int>>> signals;
  std::shared_ptr<sc_core::sc_port_b<sc_core::sc_signal_in_if<int>>> port;
};
/** @brief Copy one report without retaining kernel-owned report references. */
struct Report
{
  int id;
  int severity;
  std::string type;
  std::string message;
};
std::vector<Report> reports;
/** @brief Collect expected elaboration errors without taking default abort actions. */
void report(const sc_core::sc_report& value, const sc_core::sc_actions&)
{
  reports.push_back({value.get_id(), int(value.get_severity()), value.get_msg_type(), value.get_msg()});
}
/** @brief Reject a native semantic mismatch using a recoverable exception. */
void require(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}
/** @brief Allocate and bind one arbitrary native policy specialization. */
template <int N, sc_core::sc_port_policy Policy>
Case make_case(const std::string& label, int count, std::size_t index)
{
  Case result;
  result.label = label;
  result.maximum = N;
  result.count = count;
  {
    const std::string host_name = "port_policy_" + std::to_string(index);
    sc_core::sc_module_name name(host_name.c_str());
    result.top.reset(new Host(name));
    const int values[] = {17, 29, 43};
    for (int source = 0; source < 3; ++source)
      result.signals.emplace_back(
        new sc_core::sc_signal<int>(("source_" + std::to_string(source)).c_str(), values[source]));
    result.port = std::make_shared<sc_core::sc_port<sc_core::sc_signal_in_if<int>, N, Policy>>("required");
    for (int source = 0; source < count; ++source) result.port->bind(*result.signals[source]);
    require(result.port->bind_count() == count, "pre-elaboration binding count");
  }
  return result;
}
/** @brief Enumerate empty/partial/full/excess and unlimited binding counts. */
template <sc_core::sc_port_policy Policy>
void add_cases(std::vector<Case>& cases, const char* label)
{
  for (int count : {0, 1, 2, 3}) cases.push_back(make_case<2, Policy>(label, count, cases.size()));
  for (int count : {0, 1, 3}) cases.push_back(make_case<0, Policy>(label, count, cases.size()));
}
}  // namespace port_policy_reference

/** @brief Elaborate all cases once, verify state and emit exact native diagnostics. */
int sc_main(int, char**)
{
  using namespace sc_core;
  using namespace port_policy_reference;
  std::vector<Case> cases;
  add_cases<SC_ONE_OR_MORE_BOUND>(cases, "one");
  add_cases<SC_ZERO_OR_MORE_BOUND>(cases, "zero");
  add_cases<SC_ALL_BOUND>(cases, "all");
  auto previous = sc_report_handler::set_handler(report);
  sc_start(SC_ZERO_TIME);
  sc_report_handler::set_handler(previous);
  require(sc_get_status() == SC_PAUSED, "elaboration state");
  require(reports.size() == 8, "expected report count");
  for (std::size_t index = 0; index < cases.size(); ++index)
  {
    const Case& value = cases[index];
    require(value.port->size() == value.count, "completed binding count");
    const int expected[] = {17, 29, 43};
    std::string values;
    for (int source = 0; source < value.count; ++source)
    {
      const int actual = value.port->get_interface(source)->read();
      require(actual == expected[source], "bound interface value");
      values += (source ? "," : "") + std::to_string(actual);
    }
    std::string details;
    int count = 0;
    const std::string name = "port 'port_policy_" + std::to_string(index) + ".required'";
    for (const auto& record : reports)
    {
      if (record.message.find(name) == std::string::npos) continue;
      require(record.id == 109 && record.severity == SC_ERROR && record.type == SC_ID_COMPLETE_BINDING_,
              "report identity");
      if (count++) details += ';';
      details += record.message;
    }
    std::cout << "PORT_POLICY|" << value.label << '|' << value.maximum << '|' << value.count << '|'
              << value.port->size() << '|' << values << '|' << count << '|' << details << '\n';
  }
  std::cout << "PORT_POLICY_DONE cases=21 reports=8 time=0\n";
  return 0;
}
