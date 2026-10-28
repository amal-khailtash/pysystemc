"""Exercise process-control errors and Python report-handler ownership."""

import gc
import sys
import weakref

from amal.eda.systemc import sc_core as sc


def check_process_controls():
    """Assert reset and kill reject top-level calls without touching a live process."""
    records = []

    def worker():
        """Remain suspended so rejected controls can be checked for side effects."""
        try:
            records.append(("start", str(sc.sc_time_stamp())))
            sc.wait(5, sc.SC_NS)
        finally:
            records.append(("cleanup", str(sc.sc_time_stamp())))

    handle = sc.sc_spawn(worker, "control_error_worker")
    sc.sc_start(1, sc.SC_NS)
    assert sc.sc_get_status() == sc.SC_PAUSED
    assert records == [("start", "0 s")]

    paused_rejected = 0
    for operation in (handle.reset, handle.kill):
        try:
            operation()
        except RuntimeError as error:
            assert str(error) == "process reset/kill requires a running SystemC process"
            paused_rejected += 1
        else:
            raise AssertionError("paused top-level process control was accepted")
    assert not handle.terminated() and not handle.is_unwinding()
    assert records == [("start", "0 s")]

    sc.sc_stop()
    assert sc.sc_get_status() == sc.SC_STOPPED
    stopped_rejected = 0
    for operation in (handle.reset, handle.kill):
        try:
            operation()
        except RuntimeError as error:
            assert str(error) == "process reset/kill requires a running SystemC process"
            stopped_rejected += 1
        else:
            raise AssertionError("stopped top-level process control was accepted")
    assert not handle.terminated() and not handle.is_unwinding()
    assert records == [("start", "0 s")]
    print(f"CONTROL_ERROR_LIFETIME_OK paused={paused_rejected} stopped={stopped_rejected}")


class RaisingReportHandler:
    """Count native report dispatches and raise a stable Python diagnostic."""

    def __init__(self):
        """Start an empty call log."""
        self.calls = []

    def __call__(self, report, actions):
        """Record one borrowed report then raise the original error type."""
        self.calls.append((report.get_msg_type(), report.get_msg(), int(actions)))
        raise ValueError("report_handler_lifetime_marker")


def check_report_callback():
    """Verify exception propagation, retained lifetime, restoration and reuse."""
    handler = sc.sc_report_handler
    previous_actions = handler.set_actions(sc.SC_WARNING, sc.SC_DO_NOTHING)
    callback = RaisingReportHandler()
    callback_ref = weakref.ref(callback)
    sc.set_report_handler(callback)
    del callback
    gc.collect()
    retained = callback_ref()
    assert retained is not None

    for count in (1, 2):
        try:
            handler.report(sc.SC_WARNING, "python/callback", "message", "callback.py", count)
        except ValueError as error:
            assert type(error) is ValueError
            assert "report_handler_lifetime_marker" in str(error)
        else:
            raise AssertionError("report-handler exception did not propagate")
        assert len(retained.calls) == count
        assert retained.calls[-1][:2] == ("python/callback", "message")
    assert retained.calls[0][2] == retained.calls[1][2]

    sc.set_report_handler()
    del retained
    gc.collect()
    assert callback_ref() is None
    handler.report(sc.SC_WARNING, "python/restored", "default handler", "callback.py", 3)

    reused = []

    def replacement(report, actions):
        """Record that a fresh Python handler can be installed after restoration."""
        reused.append((report.get_msg_type(), report.get_msg(), int(actions)))

    replacement_ref = weakref.ref(replacement)
    sc.set_report_handler(replacement)
    del replacement
    gc.collect()
    assert replacement_ref() is not None

    finalized = []

    def final_handler(report, actions):
        """Record a report dispatched to the handler installed by replacement."""
        finalized.append((report.get_msg_type(), report.get_msg(), int(actions)))

    final_ref = weakref.ref(final_handler)
    sc.set_report_handler(final_handler)
    del final_handler
    gc.collect()
    assert replacement_ref() is None
    assert final_ref() is not None

    handler.report(sc.SC_WARNING, "python/reused", "new handler", "callback.py", 4)
    assert not reused
    assert len(finalized) == 1
    assert finalized[0][:2] == ("python/reused", "new handler")
    sc.set_report_handler()
    gc.collect()
    assert final_ref() is None
    handler.set_actions(sc.SC_WARNING, previous_actions)
    print("REPORT_CALLBACK_LIFETIME_OK calls=2 replaced=1 released=2 reused=1")


if len(sys.argv) != 2:
    raise SystemExit("expected controls or report scenario")
if sys.argv[1] == "controls":
    check_process_controls()
elif sys.argv[1] == "report":
    check_report_callback()
else:
    raise SystemExit("unknown scenario")
