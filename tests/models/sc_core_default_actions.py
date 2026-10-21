"""Exercise each native default-report action in an isolated interpreter."""

from pathlib import Path
import resource
import sys
import signal
from amal.eda.systemc import sc_core as sc

resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
# cppyy installs a diagnostic SIGABRT handler; restore native termination for
# this isolated terminal-action reference instead of accepting its custom status.
signal.signal(signal.SIGABRT, signal.SIG_DFL)
mode = sys.argv[1]
reports = []


def capture(report, actions):
    """Copy the source report before invoking native default actions."""
    reports.append(sc.sc_report(report))


sc.set_report_handler(capture)
sc.sc_report_handler.report(sc.SC_WARNING, "actions/test", "message", "actions.cpp", 17)
sc.set_report_handler()
report = reports.pop()
actions = {
    "display": sc.SC_DISPLAY,
    "log": sc.SC_LOG,
    "interrupt": sc.SC_INTERRUPT,
    "throw": sc.SC_THROW,
    "stop": sc.SC_STOP,
    "stop_throw": sc.SC_STOP | sc.SC_THROW,
    "abort": sc.SC_ABORT,
    "combined": sc.SC_DISPLAY | sc.SC_LOG | sc.SC_INTERRUPT | sc.SC_THROW,
    "none": sc.SC_DO_NOTHING,
}
if mode in ("log", "combined"):
    assert sc.sc_report_handler.set_log_file_name("actions.log")
print("DEFAULT_ACTION_ENTER", mode, flush=True)
try:
    sc.sc_report_handler.default_handler(report, actions[mode])
except Exception as error:
    assert mode in ("throw", "combined", "stop_throw") and "actions/test: message" in str(error)
    print("DEFAULT_ACTION_THROW", mode)
else:
    assert mode not in ("abort", "throw", "combined", "stop_throw")
if mode in ("log", "combined"):
    assert not sc.sc_report_close_default_log()
    assert "Warning: actions/test: message\nIn file: actions.cpp:17" in Path("actions.log").read_text()
if mode in ("stop", "stop_throw"):
    assert sc.sc_get_status() == sc.SC_STOPPED
else:
    assert sc.sc_get_status() == sc.SC_ELABORATION
print("DEFAULT_ACTION_RETURN", mode)
