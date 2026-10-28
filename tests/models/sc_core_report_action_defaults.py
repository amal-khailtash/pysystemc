"""Compare defaulted report-action setters and fallback policy behavior."""

from amal.eda.systemc import sc_core as sc


handler = sc.sc_report_handler
records = []


def capture(report, actions):
    """Copy message type, severity and resolved action bits during dispatch."""
    if report.get_msg_type().startswith("action/default/"):
        records.append((report.get_msg_type(), int(report.get_severity()), int(actions)))


def attempt_default(label, seed_previous, default_call, query_call, restore_call, report_call):
    """Run one omitted-actions setter, inspect its scope, and emit its report."""
    try:
        previous = int(default_call())
        accepted = 1
    except TypeError:
        previous = -1
        accepted = 0
    current = int(query_call())
    restore_call(current)
    report_call()
    assert records[-1][0] == label, records[-1]
    print(
        "REPORT_ACTION_DEFAULT",
        label.rsplit("/", 1)[-1],
        accepted,
        int(seed_previous),
        previous,
        current,
        records[-1][2],
    )


sc.set_report_handler(capture)

severity_seed = handler.set_actions(sc.SC_INFO, sc.SC_LOG)
attempt_default(
    "action/default/severity",
    severity_seed,
    lambda: handler.set_actions(sc.SC_INFO),
    lambda: handler.set_actions(sc.SC_INFO, sc.SC_UNSPECIFIED),
    lambda actions: handler.set_actions(sc.SC_INFO, actions),
    lambda: handler.report(sc.SC_INFO, "action/default/severity", "severity", "defaults.py", 11),
)

warning_actions = handler.set_actions(sc.SC_WARNING, sc.SC_CACHE_REPORT)
message_seed_type = "action/default/message"
message_seed = handler.set_actions(message_seed_type, sc.SC_LOG)
attempt_default(
    message_seed_type,
    message_seed,
    lambda: handler.set_actions(message_seed_type),
    lambda: handler.set_actions(message_seed_type, sc.SC_UNSPECIFIED),
    lambda actions: handler.set_actions(message_seed_type, actions),
    lambda: handler.report(sc.SC_WARNING, message_seed_type, "message", "defaults.py", 12),
)

error_actions = handler.set_actions(sc.SC_ERROR, sc.SC_LOG)
specific_seed_type = "action/default/specific"
specific_message_seed = handler.set_actions(specific_seed_type, sc.SC_CACHE_REPORT)
specific_seed = handler.set_actions(specific_seed_type, sc.SC_ERROR, sc.SC_DISPLAY)
attempt_default(
    specific_seed_type,
    specific_seed,
    lambda: handler.set_actions(specific_seed_type, sc.SC_ERROR),
    lambda: handler.set_actions(specific_seed_type, sc.SC_ERROR, sc.SC_UNSPECIFIED),
    lambda actions: handler.set_actions(specific_seed_type, sc.SC_ERROR, actions),
    lambda: handler.report(sc.SC_ERROR, specific_seed_type, "specific", "defaults.py", 13),
)

assert len(records) == 3, records
sc.set_report_handler()
handler.set_actions(sc.SC_INFO, severity_seed)
handler.set_actions(sc.SC_WARNING, warning_actions)
handler.set_actions(message_seed_type, message_seed)
handler.set_actions(sc.SC_ERROR, error_actions)
handler.set_actions(specific_seed_type, specific_message_seed)
handler.set_actions(specific_seed_type, sc.SC_ERROR, specific_seed)
print("REPORT_ACTION_DEFAULT_CASES", len(records))
