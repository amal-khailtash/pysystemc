/** @file sc_core_all_bound_policy.cpp
 *  @brief Native reference for bounded and unbounded SC_ALL_BOUND ports.
 */
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

namespace
{
/** @brief Expose the protected native module constructor for this reference. */
struct AllBoundTop : sc_core::sc_module
{
  /** @brief Construct a hierarchy owner from the caller's live module name. */
  explicit AllBoundTop(sc_core::sc_module_name name) : sc_module(name)
  {
  }
};

struct report_record
{
  int id;
  int severity;
  std::string type;
  std::string message;
};

std::vector<report_record> reports;

/** @brief Copy an elaboration report while suppressing its default action. */
void capture_report(const sc_core::sc_report& report, const sc_core::sc_actions&)
{
  reports.push_back(
    {report.get_id(), static_cast<int>(report.get_severity()), report.get_msg_type(), report.get_msg()});
}

/** @brief Reject a failed native semantic check with an actionable diagnostic. */
void require(bool condition, const std::string& message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Exercise one bounded or unbounded port binding count. */
template <int N>
void check_case(const std::string& scenario, int binding_count)
{
  using port_type = sc_core::sc_port<sc_core::sc_signal_in_if<int>, N, sc_core::SC_ALL_BOUND>;
  std::unique_ptr<AllBoundTop> top;
  std::unique_ptr<sc_core::sc_signal<int>> left;
  std::unique_ptr<sc_core::sc_signal<int>> right;
  std::unique_ptr<port_type> port;
  {
    sc_core::sc_module_name module_name("all_bound");
    top.reset(new AllBoundTop(module_name));
    left.reset(new sc_core::sc_signal<int>("left", 17));
    right.reset(new sc_core::sc_signal<int>("right", 29));
    port.reset(new port_type("required"));
    sc_core::sc_signal<int>* channels[] = {left.get(), right.get()};
    for (int index = 0; index < binding_count; ++index) port->bind(*channels[index]);
    require(port->bind_count() == binding_count, "pre-elaboration bind count");
  }

  auto previous = sc_core::sc_report_handler::set_handler(capture_report);
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  sc_core::sc_report_handler::set_handler(previous);
  require(sc_core::sc_get_status() == sc_core::SC_PAUSED, "zero-time elaboration status");
  require(port->size() == binding_count, "completed port size");

  std::vector<int> values;
  for (int index = 0; index < binding_count; ++index) values.push_back(port->get_interface(index)->read());
  const int expected_values[] = {17, 29};
  for (int index = 0; index < binding_count; ++index)
    require(values[static_cast<std::size_t>(index)] == expected_values[index], "bound interface value");

  if ((N == 2 && binding_count == 2) || (N == 0 && binding_count == 1))
  {
    require(reports.empty(), "valid SC_ALL_BOUND count reported an error");
  }
  else
  {
    const std::string expected = std::to_string(binding_count) + " actual binds is less than required " +
                                 std::to_string(N) + ": port 'all_bound.required' (sc_port)";
    require(reports.size() == 1, "invalid SC_ALL_BOUND count report cardinality");
    require(reports[0].id == 109, "SC_ALL_BOUND report ID");
    require(reports[0].severity == static_cast<int>(sc_core::SC_ERROR), "SC_ALL_BOUND report severity");
    require(reports[0].type == sc_core::SC_ID_COMPLETE_BINDING_, "SC_ALL_BOUND report type");
    require(reports[0].message == expected, "SC_ALL_BOUND report diagnostic");
  }

  std::cout << "ALL_BOUND_REFERENCE|" << scenario << '|' << N << '|' << port->bind_count() << '|' << port->size()
            << '|';
  for (std::size_t index = 0; index < values.size(); ++index)
  {
    if (index) std::cout << ',';
    std::cout << values[index];
  }
  std::cout << '|' << reports.size();
  for (std::size_t index = 0; index < reports.size(); ++index)
  {
    const auto& report = reports[index];
    std::cout << '|' << report.id << ';' << report.severity << ';' << report.type << ';' << report.message;
  }
  std::cout << '\n';
}
}  // namespace

/** @brief Dispatch a fresh-kernel native SC_ALL_BOUND policy case. */
int sc_main(int argc, char** argv)
{
  if (argc != 2) throw std::runtime_error("expected one SC_ALL_BOUND scenario");
  const std::string scenario(argv[1]);
  if (scenario == "bounded_full")
    check_case<2>(scenario, 2);
  else if (scenario == "bounded_under")
    check_case<2>(scenario, 1);
  else if (scenario == "bounded_none")
    check_case<2>(scenario, 0);
  else if (scenario == "unbounded_one")
    check_case<0>(scenario, 1);
  else if (scenario == "unbounded_none")
    check_case<0>(scenario, 0);
  else
    throw std::runtime_error("unknown SC_ALL_BOUND scenario: " + scenario);
  return 0;
}
