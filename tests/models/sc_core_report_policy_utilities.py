"""Exercise report-handler utility contracts against a native C++ reference."""

import ctypes

from amal.eda.systemc import sc_core as sc
import cppyy


h = sc.sc_report_handler

# A null name clears the stored filename and permits a fresh name to be set.
first_installed = h.set_log_file_name("policy_first.log")
second_refused = h.set_log_file_name("policy_refused.log")
first_retained = h.get_log_file_name() == "policy_first.log"
first_clear_return = h.set_log_file_name(cppyy.nullptr)
first_cleared = not h.get_log_file_name()
second_installed = h.set_log_file_name("policy_second.log")
second_clear_return = h.set_log_file_name(None)
second_cleared = not h.get_log_file_name()
assert (
    first_installed
    and not second_refused
    and first_retained
    and not first_clear_return
    and first_cleared
    and second_installed
    and not second_clear_return
    and second_cleared
)
print(
    "REPORT_POLICY_LOG",
    int(first_installed),
    int(second_refused),
    int(first_retained),
    int(first_clear_return),
    int(first_cleared),
    int(second_installed),
    int(second_clear_return),
    int(second_cleared),
)

# An absent legacy report ID stays unsuppressed even after a suppression request.
missing_id = 61998
assert not sc.sc_report.is_suppressed(missing_id)
sc.sc_report.suppress_id(missing_id, True)
still_unsuppressed = not sc.sc_report.is_suppressed(missing_id)
assert still_unsuppressed
print("REPORT_POLICY_ABSENT_ID", 0, int(still_unsuppressed))

# New action bits are independent single bits and reach the report callback.
action_one = int(h.get_new_action_id())
action_two = int(h.get_new_action_id())
valid_bits = all(value != 0 and value & (value - 1) == 0 for value in (action_one, action_two))
disjoint_bits = action_one & action_two == 0
message_type = "report/policy_generated_action"
seen_actions = []


def capture_action(report, actions):
    """Record the native action mask for the generated-action report."""
    if report.get_msg_type() == message_type:
        seen_actions.append(int(actions))


h.set_actions(message_type, action_two)
sc.set_report_handler(capture_action)
h.report(sc.SC_INFO, message_type, "generated action", "report_policy.cpp", 12)
sc.set_report_handler()
callback_received_bit = seen_actions == [action_two]
assert valid_bits and disjoint_bits and callback_received_bit
print(
    "REPORT_POLICY_NEW_ACTION",
    int(valid_bits),
    int(disjoint_bits),
    int(callback_received_bit),
)

# Allocation is finite and exhaustion must remain a stable native zero result.
allocated = action_one | action_two
remaining = 0
for _ in range(ctypes.sizeof(ctypes.c_uint) * 8 + 1):
    action = int(h.get_new_action_id())
    if action == sc.SC_UNSPECIFIED:
        break
    assert action & (action - 1) == 0 and not action & allocated
    allocated |= action
    remaining += 1
else:
    raise AssertionError("native action allocation did not exhaust")
assert h.get_new_action_id() == sc.SC_UNSPECIFIED
assert seen_actions == [action_two]
print("REPORT_POLICY_ACTION_EXHAUSTION", remaining, 1, 1)
