"""Compare report-handler overloads, policy precedence, caches, logs and release."""

from pathlib import Path
from amal.eda.systemc import sc_core as sc

h = sc.sc_report_handler
records = []


def capture(report, actions):
    """Retain copied scalar evidence, never a borrowed report across cache mutation."""
    if report.get_msg_type().startswith("handler/"):
        records.append((report.get_msg_type(), report.get_msg(), int(actions), report.get_verbosity()))


previous = h.get_handler()
assert previous
sc.set_report_handler(capture)
assert h.get_handler()
h.initialize()
assert h.get_count("handler/policy") == 0
old = h.set_actions(sc.SC_WARNING, sc.SC_DISPLAY)
assert h.set_actions("handler/policy", sc.SC_LOG) == sc.SC_UNSPECIFIED
assert h.set_actions("handler/policy", sc.SC_WARNING, sc.SC_CACHE_REPORT) == sc.SC_UNSPECIFIED
h.report(sc.SC_WARNING, "handler/policy", "specific", "handler.cpp", 12)
assert h.get_cached_report().get_msg() == "specific"
copy = sc.sc_report(h.get_cached_report())
h.clear_cached_report()
assert not h.get_cached_report() and copy.get_msg() == "specific"
h.set_actions("handler/policy", sc.SC_WARNING, sc.SC_UNSPECIFIED)
h.report(sc.SC_WARNING, "handler/policy", "type", "handler.cpp", 12)
h.set_actions("handler/policy", sc.SC_UNSPECIFIED)
h.report(sc.SC_WARNING, "handler/policy", "severity", "handler.cpp", 12)
assert h.suppress(sc.SC_DISPLAY) == 0
assert h.force(sc.SC_LOG) == 0
h.report(sc.SC_WARNING, "handler/policy", "masks", "handler.cpp", 12)
assert h.suppress() == sc.SC_DISPLAY and h.force() == sc.SC_LOG
h.set_actions(sc.SC_WARNING, old)
assert h.get_count("handler/policy") == h.get_count("handler/policy", sc.SC_WARNING) == h.get_count(sc.SC_WARNING) == 4
verbosity = h.set_verbosity_level(sc.SC_LOW)
h.report(sc.SC_INFO, "handler/verbosity", "hidden", "handler.cpp", 12)
h.report(sc.SC_INFO, "handler/verbosity", "hidden", sc.SC_HIGH, "handler.cpp", 12)
h.report(sc.SC_INFO, "handler/verbosity", "visible", sc.SC_LOW, "handler.cpp", 12)
assert h.get_count("handler/verbosity") == 1 and h.get_verbosity_level() == sc.SC_LOW
h.set_verbosity_level(verbosity)
for scope in (sc.SC_INFO, "handler/limit", ("handler/limit", sc.SC_INFO)):
    args = scope if isinstance(scope, tuple) else (scope,)
    h.stop_after(*args, 2)
    h.report(sc.SC_INFO, "handler/limit", "limit", "handler.cpp", 12)
    h.report(sc.SC_INFO, "handler/limit", "limit", "handler.cpp", 12)
    assert records[-1][2] & sc.SC_STOP
    assert h.stop_after(*args) == 2, (scope, h.stop_after(*args, -1))
catch = h.get_catch_actions()
assert h.set_catch_actions(sc.SC_DO_NOTHING) == catch
assert h.get_catch_actions() == sc.SC_DO_NOTHING
h.set_catch_actions(catch)
action = h.get_new_action_id()
assert action and action & (action - 1) == 0
sc.set_report_handler()
assert h.set_handler(previous)
h.report(sc.SC_INFO, "handler/no_action", "restored", "handler.cpp", 12)
assert h.set_handler(None)
assert h.set_handler(h.get_handler())
try:
    h.set_handler(object())
except TypeError:
    pass
else:
    raise AssertionError("invalid handler accepted")
assert h.set_log_file_name("handler.log")
assert not h.set_log_file_name("other.log") and h.get_log_file_name() == "handler.log"
h.set_actions("handler/log", sc.SC_LOG)
h.report(sc.SC_INFO, "handler/log", "written", "handler.cpp", 12)
assert not sc.sc_report_close_default_log()
assert not h.get_log_file_name()
assert "handler/log: written" in Path("handler.log").read_text()
assert sc.sc_report_close_default_log()
h.default_handler(copy, sc.SC_DO_NOTHING)


def cache_worker():
    """Verify process-local cache lifetime and copy survival after clear."""
    h.set_actions("handler/local", sc.SC_CACHE_REPORT)
    sc.set_report_handler(lambda report, actions: None)
    h.report(sc.SC_WARNING, "handler/local", "local", "handler.cpp", 12)
    cached = h.get_cached_report()
    retained = sc.sc_report(cached)
    assert cached.get_process_name() == "cache_worker"
    h.clear_cached_report()
    assert not h.get_cached_report() and retained.get_msg() == "local"
    sc.set_report_handler()


process = sc.sc_spawn(cache_worker, "cache_worker")
sc.sc_start(1, sc.SC_NS)
assert process.terminated() and not h.get_cached_report()
try:
    h.default_handler(copy, sc.SC_THROW)
except Exception as error:
    assert "specific" in str(error)
else:
    raise AssertionError("default throw action returned")
h.default_handler(copy, sc.SC_STOP)
assert sc.sc_get_status() == sc.SC_STOPPED
h.initialize()
assert h.get_count("handler/policy") == 0
# Destroy copies before release: report message definitions are kernel-owned.
copy.__destruct__()
h.release()
h.initialize()
assert h.get_count("handler/policy") == 0
for row in records:
    print("HANDLER_REFERENCE", *row)
print("HANDLER_FINAL_REFERENCE", action, len(records))
