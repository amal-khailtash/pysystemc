/** @file sc_core_report_policy_utilities.cpp
 * @brief Native oracle for report-handler filename, legacy suppression and action IDs.
 * @details Uses only safe report severities and non-null report arguments.
 */
#include <climits>
#include <systemc>
#include <vector>

using namespace sc_core;

namespace
{
constexpr int missing_id = 61998;
constexpr const char* action_message = "report/policy_generated_action";
std::vector<unsigned> seen_actions;

/** @brief Record the action mask supplied for the generated-action report. */
void capture_action(const sc_report& report, const sc_actions& actions)
{
  if (std::string(report.get_msg_type()) == action_message) seen_actions.push_back(actions);
}
}  // namespace

/** @brief Compare native report-handler utility behavior with the Python-facing contracts. */
int sc_main(int, char**)
{
  using h = sc_report_handler;

  const bool first_installed = h::set_log_file_name("policy_first.log");
  const bool second_refused = h::set_log_file_name("policy_refused.log");
  const bool first_retained = std::string(h::get_log_file_name()) == "policy_first.log";
  const bool first_clear_return = h::set_log_file_name(nullptr);
  const bool first_cleared = h::get_log_file_name() == nullptr;
  const bool second_installed = h::set_log_file_name("policy_second.log");
  const bool second_clear_return = h::set_log_file_name(nullptr);
  const bool second_cleared = h::get_log_file_name() == nullptr;
  if (!first_installed || second_refused || !first_retained || first_clear_return || !first_cleared ||
      !second_installed || second_clear_return || !second_cleared)
    return 2;
  std::cout << "REPORT_POLICY_LOG " << first_installed << " " << second_refused << " " << first_retained << " "
            << first_clear_return << " " << first_cleared << " " << second_installed << " " << second_clear_return
            << " " << second_cleared << "\n";

  if (sc_report::is_suppressed(missing_id)) return 3;
  sc_report::suppress_id(missing_id, true);
  const bool still_unsuppressed = !sc_report::is_suppressed(missing_id);
  if (!still_unsuppressed) return 4;
  std::cout << "REPORT_POLICY_ABSENT_ID 0 " << still_unsuppressed << "\n";

  const sc_actions action_one = h::get_new_action_id();
  const sc_actions action_two = h::get_new_action_id();
  const bool valid_bits =
    action_one != 0 && (action_one & (action_one - 1)) == 0 && action_two != 0 && (action_two & (action_two - 1)) == 0;
  const bool disjoint_bits = (action_one & action_two) == 0;
  if (!valid_bits || !disjoint_bits) return 5;
  h::set_actions(action_message, action_two);
  const auto previous_handler = h::set_handler(capture_action);
  h::report(SC_INFO, action_message, "generated action", "report_policy.cpp", 12);
  h::set_handler(previous_handler);
  const bool callback_received_bit = seen_actions.size() == 1 && seen_actions.front() == action_two;
  if (!callback_received_bit) return 6;
  std::cout << "REPORT_POLICY_NEW_ACTION " << valid_bits << " " << disjoint_bits << " " << callback_received_bit
            << "\n";
  sc_actions allocated = action_one | action_two;
  unsigned remaining = 0;
  bool exhausted = false;
  for (unsigned index = 0; index <= sizeof(sc_actions) * CHAR_BIT; ++index)
  {
    const auto action = h::get_new_action_id();
    if (action == SC_UNSPECIFIED)
    {
      exhausted = true;
      break;
    }
    if ((action & (action - 1)) != 0 || (action & allocated) != 0) return 7;
    allocated |= action;
    ++remaining;
  }
  if (!exhausted || h::get_new_action_id() != SC_UNSPECIFIED || seen_actions.size() != 1) return 8;
  std::cout << "REPORT_POLICY_ACTION_EXHAUSTION " << remaining << " 1 1\n";
  return 0;
}
