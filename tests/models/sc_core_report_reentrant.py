"""Exercise report-handler replacement and restoration inside callbacks."""

import gc
import sys
import weakref

from amal.eda.systemc import sc_core as sc


class Replacement:
    """Record reports delivered after the active handler replaces itself."""

    def __init__(self, calls):
        """Keep the shared report-call log."""
        self.calls = calls

    def __call__(self, report, actions):
        """Record one nested report dispatch."""
        self.calls.append((report.get_msg_type(), report.get_msg(), int(actions)))


class Replacing:
    """Replace the installed handler, emit a nested report, and fail."""

    def __init__(self, replacement, events, copies):
        """Store callback dependencies and initialize dispatch state."""
        self.replacement = replacement
        self.events = events
        self.copies = copies
        self.calls = 0
        self.after_replace = 0

    def __call__(self, report, actions):
        """Use own state after replacement and copy the cached report."""
        self.calls += 1
        self.copies.append(sc.sc_report(report))
        sc.set_report_handler(self.replacement)
        self.after_replace += 1
        self.events.append((self.calls, self.after_replace))
        sc.sc_report_handler.report(sc.SC_INFO, "reentrant/nested", "nested report", "reentrant.py", 2)
        sc.sc_report_handler.clear_cached_report()
        assert not sc.sc_report_handler.get_cached_report()
        self.events.append(("copy", self.copies[0].get_msg()))
        raise ValueError("reentrant original callback error")


class Restoring:
    """Restore the native handler, then use own state and raise."""

    def __init__(self, events):
        """Initialize callback evidence and state counters."""
        self.events = events
        self.calls = 0
        self.after_restore = 0

    def __call__(self, report, actions):
        """Emit a suppressed nested report after restoring native dispatch."""
        self.calls += 1
        sc.set_report_handler()
        self.after_restore += 1
        self.events.append((report.get_msg_type(), self.calls, self.after_restore))
        sc.sc_report_handler.report(sc.SC_INFO, "reentrant/suppressed", "suppressed", "reentrant.py", 4)
        raise ValueError("restored original callback error")


def ordinary_error_prefix(handler):
    """Capture cppyy's ordinary report callback error formatting for comparison."""
    handler.set_actions("reentrant/baseline", sc.SC_INFO, sc.SC_CACHE_REPORT)

    def fail(report, actions):
        """Raise the baseline Python report-handler error."""
        raise ValueError("ordinary callback error")

    callback_ref = weakref.ref(fail)
    sc.set_report_handler(fail)
    del fail
    try:
        handler.report(sc.SC_INFO, "reentrant/baseline", "baseline", "reentrant.py", 5)
    except ValueError as error:
        failure = error
    else:
        raise AssertionError("ordinary callback error did not propagate")
    assert type(failure) is ValueError and "ordinary callback error" in str(failure)
    assert failure.__cause__ is None and failure.__context__ is None
    prefix = str(failure).split("ordinary callback error", 1)[0]
    sc.set_report_handler()
    del failure
    gc.collect()
    assert callback_ref() is None
    return prefix


def verify_failure(failure, message):
    """Check callback error type, diagnostic, and empty chaining context."""
    assert type(failure) is ValueError
    assert message in str(failure)
    assert failure.__cause__ is None and failure.__context__ is None


def check_replace():
    """Exercise replacement, nested dispatch, cache clearing, and owner release."""
    handler = sc.sc_report_handler
    baseline_prefix = ordinary_error_prefix(handler)
    handler.set_actions("reentrant/outer", sc.SC_INFO, sc.SC_CACHE_REPORT)
    handler.set_actions("reentrant/nested", sc.SC_INFO, sc.SC_DISPLAY)
    events, copies, replacement_calls = [], [], []
    replacement = Replacement(replacement_calls)
    replacement_ref = weakref.ref(replacement)
    original = Replacing(replacement, events, copies)
    original_ref = weakref.ref(original)
    sc.set_report_handler(original)
    del original, replacement

    failure = None
    try:
        handler.report(sc.SC_INFO, "reentrant/outer", "outer report", "reentrant.py", 1)
    except ValueError as error:
        failure = error
    else:
        raise AssertionError("reentrant callback error did not propagate")

    verify_failure(failure, "reentrant original callback error")
    assert str(failure).split("reentrant original callback error", 1)[0] == baseline_prefix
    assert events == [(1, 1), ("copy", "outer report")]
    assert len(replacement_calls) == 1 and replacement_calls[0][:2] == ("reentrant/nested", "nested report")
    assert copies[0].get_msg() == "outer report"
    assert original_ref() is None
    assert replacement_ref() is not None

    del failure
    gc.collect()
    assert original_ref() is None
    assert replacement_ref() is not None
    sc.set_report_handler()
    gc.collect()
    assert replacement_ref() is None
    print("REPORT_REENTRANT_OK replace nested=1 original_released=1 replacement_released=1")


def check_restore():
    """Exercise self-restoration and a nested report suppressed by native policy."""
    handler = sc.sc_report_handler
    baseline_prefix = ordinary_error_prefix(handler)
    handler.set_actions("reentrant/outer", sc.SC_INFO, sc.SC_CACHE_REPORT)
    handler.set_actions("reentrant/suppressed", sc.SC_INFO, sc.SC_DO_NOTHING)
    events = []
    original = Restoring(events)
    original_ref = weakref.ref(original)
    sc.set_report_handler(original)
    del original

    failure = None
    try:
        handler.report(sc.SC_INFO, "reentrant/outer", "outer report", "reentrant.py", 3)
    except ValueError as error:
        failure = error
    else:
        raise AssertionError("restoring callback error did not propagate")

    verify_failure(failure, "restored original callback error")
    assert str(failure).split("restored original callback error", 1)[0] == baseline_prefix
    assert events == [("reentrant/outer", 1, 1)]
    assert handler.get_count("reentrant/suppressed") == 1
    assert original_ref() is None
    del failure
    gc.collect()
    assert original_ref() is None
    assert sc.sc_report_handler.get_cached_report()
    sc.sc_report_handler.clear_cached_report()
    print("REPORT_REENTRANT_OK restore suppressed=1 original_released=1")


if len(sys.argv) != 2:
    raise SystemExit("expected replace or restore scenario")
if sys.argv[1] == "replace":
    check_replace()
elif sys.argv[1] == "restore":
    check_restore()
else:
    raise SystemExit("unknown report reentrant scenario")
