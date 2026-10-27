"""Audit plain dynamic-process callback ownership after collection and errors."""

import gc
import sys
import weakref

from amal.eda.systemc import sc_core as sc


class RetainedCallback:
    """Record execution and expose callback lifetime to weak references."""

    def __init__(self, *, fail=False):
        """Store callback mode and initialize its call log."""
        self.fail = fail
        self.calls = []

    def __call__(self):
        """Record native dispatch, optionally raising a stable Python error."""
        self.calls.append(str(sc.sc_time_stamp()))
        if self.fail:
            raise LookupError("plain_spawn_callback_lifetime_marker")


def run_success():
    """Prove a plain spawn callback survives GC, executes and remains retained."""
    callback = RetainedCallback()
    reference = weakref.ref(callback)
    handle = sc.sc_spawn(callback, "plain_spawn_lifetime")
    del callback
    gc.collect()
    retained = reference()
    assert retained is not None

    sc.sc_start(1, sc.SC_NS)
    assert handle.terminated()
    assert retained.calls == ["0 s"]
    sc.sc_stop()
    del retained
    gc.collect()
    assert reference() is not None, "kernel-owned spawn callback was released after stop"
    print("PLAIN_SPAWN_CALLBACK_LIFETIME_OK calls=1 retained_after_stop=1")


def run_error():
    """Prove callback failure propagates and does not lose native-lifetime ownership."""
    callback = RetainedCallback(fail=True)
    reference = weakref.ref(callback)
    sc.sc_spawn(callback, "plain_spawn_callback_error")
    del callback
    gc.collect()
    retained = reference()
    assert retained is not None

    try:
        sc.sc_start(1, sc.SC_NS)
    except sc.sc_report as error:
        assert error.get_id() == 549
        assert "plain_spawn_callback_lifetime_marker" in error.get_msg()
    else:
        raise AssertionError("plain spawn callback error did not propagate")

    assert retained.calls == ["0 s"]
    del retained
    gc.collect()
    assert reference() is not None, "callback error released a live native process callback"
    print("PLAIN_SPAWN_CALLBACK_ERROR_OK calls=1 retained_after_error=1")


if len(sys.argv) != 2:
    raise SystemExit("expected success or error scenario")
if sys.argv[1] == "success":
    run_success()
elif sys.argv[1] == "error":
    run_error()
else:
    raise SystemExit("unknown scenario")
