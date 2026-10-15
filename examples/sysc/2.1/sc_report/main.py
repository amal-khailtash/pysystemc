"""Port of Accellera SystemC 2.3.4 sysc/2.1/sc_report (Apache-2.0).

See upstream LICENSE/NOTICE. Demonstrates severity/id/action precedence,
user-defined actions, and temporary suppression/forcing with a Python handler.
"""

from amal.eda.systemc import main, sc_core as sc

USER_ACTIONS = []
SEVERITIES = (sc.SC_INFO, sc.SC_WARNING, sc.SC_ERROR, sc.SC_FATAL)
NAMES = ("INFO", "WARNING", "ERROR", "FATAL")


def allocate_user_actions():
    """Allocate all available user bits and retain the first six."""
    reserved = {
        sc.SC_UNSPECIFIED,
        sc.SC_DO_NOTHING,
        sc.SC_THROW,
        sc.SC_LOG,
        sc.SC_DISPLAY,
        sc.SC_CACHE_REPORT,
        sc.SC_STOP,
        sc.SC_ABORT,
    }
    for count in range(1000):
        action = sc.sc_report_handler.get_new_action_id()
        if action == sc.SC_UNSPECIFIED:
            print(f"We got {count} user-defined actions")
            break
        assert action not in reserved
        if count < 6:
            assert action not in USER_ACTIONS
            USER_ACTIONS.append(action)


def dump_all_handler(report, actions):
    """Print native report fields and resolved actions without executing them."""
    known = {
        int(sc.SC_DO_NOTHING): "do-nothing",
        int(sc.SC_THROW): "throw",
        int(sc.SC_LOG): "log",
        int(sc.SC_DISPLAY): "display",
        int(sc.SC_CACHE_REPORT): "cache-report",
        int(sc.SC_INTERRUPT): "interrupt",
        int(sc.SC_STOP): "stop",
        int(sc.SC_ABORT): "abort",
    }
    known.update({int(value): f"usr{index + 1}" for index, value in enumerate(USER_ACTIONS)})
    labels = [known.get(1 << bit, "UNKNOWN") for bit in range(32) if int(actions) & (1 << bit)]
    print(
        f"report: {report.get_msg_type()} {NAMES[int(report.get_severity())]} --> "
        + "".join(f" {label}" for label in labels)
    )
    process = report.get_process_name() or "<none>"
    print(
        f" msg={report.get_msg()} file={report.get_file_name()}"
        f" line {report.get_line_number()} time={report.get_time()} process={process}"
    )


def query_rules(identifier):
    """Emit all severities for one message identifier."""
    for line, severity in enumerate(SEVERITIES):
        sc.sc_report_handler.report(
            severity, identifier, f"extra_msg_for_{NAMES[line].lower()}", "no_specific_file", line
        )
    print()


def sc_main(args):
    """Exercise the original action-rule sequence and restore the handler."""
    handler = sc.sc_report_handler
    allocate_user_actions()
    sc.set_report_handler(dump_all_handler)
    try:
        handler.stop_after(sc.SC_ERROR, 0)
        handler.stop_after(sc.SC_FATAL, 0)
        print("Default settings for ID4")
        query_rules("ID4")
        print("Specific settings for ID1..ID3")
        for severity, action in zip(SEVERITIES, USER_ACTIONS[1:5]):
            handler.set_actions(severity, action)
        handler.set_actions("ID2", USER_ACTIONS[0])
        handler.set_actions("ID3", sc.SC_INFO, USER_ACTIONS[4])
        handler.set_actions("ID3", sc.SC_ERROR, USER_ACTIONS[0])
        for identifier in ("ID1", "ID2", "ID3"):
            query_rules(identifier)
        print("temporarily suppress usr4")
        sc.sc_start(1, sc.SC_NS)
        handler.suppress(USER_ACTIONS[2])
        assert handler.suppress(USER_ACTIONS[3]) == USER_ACTIONS[2]
        query_rules("ID1")
        assert handler.suppress() == USER_ACTIONS[3]
        query_rules("ID1")
        print("temporarily force usr1")
        sc.sc_start(1, sc.SC_NS)
        handler.force(USER_ACTIONS[1])
        assert handler.force(USER_ACTIONS[0]) == USER_ACTIONS[1]
        query_rules("ID1")
        assert handler.force() == USER_ACTIONS[0]
        query_rules("ID1")
        print("temporarily suppress {usr3,usr4} and force {usr1,usr3}")
        sc.sc_start(1, sc.SC_NS)
        handler.force(USER_ACTIONS[0] | USER_ACTIONS[2])
        handler.suppress(USER_ACTIONS[2] | USER_ACTIONS[3])
        query_rules("ID1")
        handler.force()
        handler.suppress()
        query_rules("ID1")
    finally:
        sc.set_report_handler()
    return 0


if __name__ == "__main__":
    main()
