"""Compare current-process self-suspension, reset and kill with native SystemC."""

import sys

from amal.eda.systemc import sc_core as sc


mode = sys.argv[1]
if mode not in {"resume", "reset", "kill"}:
    raise ValueError(f"unsupported self-suspend mode: {mode}")

module_name = sc.sc_module_name("self_suspend")
module = sc.sc_module(module_name)
records = []
event_records = []


def worker():
    """Suspend through the retained current handle and record unwind cleanup."""
    records.append(("entry", str(sc.sc_time_stamp())))
    try:
        current = sc.sc_get_current_process_handle()
        current.suspend()
        records.append(("resume", str(sc.sc_time_stamp())))
    finally:
        records.append(("cleanup", str(sc.sc_time_stamp())))


worker_handle = sc.sc_spawn(worker, "worker")
reset_event = worker_handle.reset_event()
termination_event = worker_handle.terminated_event()


def watch_reset():
    """Record execution after the worker reset event is notified."""
    sc.wait(reset_event)
    event_records.append(("reset", str(sc.sc_time_stamp())))


def watch_termination():
    """Record execution after the worker termination event is notified."""
    sc.wait(termination_event)
    event_records.append(("terminated", str(sc.sc_time_stamp())))


reset_watcher = sc.sc_spawn(watch_reset, "reset_watcher") if mode == "reset" else None
termination_watcher = sc.sc_spawn(watch_termination, "termination_watcher")


def controller():
    """Apply the selected operation while the worker is self-suspended."""
    sc.wait(1, sc.SC_NS)
    if mode == "resume":
        worker_handle.resume()
    elif mode == "reset":
        worker_handle.reset()
        sc.wait(1, sc.SC_NS)
        worker_handle.resume()
    else:
        worker_handle.kill()


controller_handle = sc.sc_spawn(controller, "controller")
del module_name
sc.sc_start(4, sc.SC_NS)

assert worker_handle.terminated(), (mode, records)
assert controller_handle.terminated(), (mode, records)
assert termination_watcher.terminated(), (mode, event_records)
if reset_watcher is not None:
    assert reset_watcher.terminated(), (mode, event_records)
expected = {
    "resume": [("entry", "0 s"), ("resume", "1 ns"), ("cleanup", "1 ns")],
    "reset": [
        ("entry", "0 s"),
        ("cleanup", "1 ns"),
        ("entry", "1 ns"),
        ("resume", "2 ns"),
        ("cleanup", "2 ns"),
    ],
    "kill": [("entry", "0 s"), ("cleanup", "1 ns")],
}[mode]
assert records == expected, (mode, records)
expected_events = {
    "resume": [("terminated", "1 ns")],
    "reset": [("reset", "1 ns"), ("terminated", "2 ns")],
    "kill": [("terminated", "1 ns")],
}[mode]
assert event_records == expected_events, (mode, event_records)
sc.sc_stop()
for label, time in records:
    print(f"SELF_SUSPEND_REFERENCE {mode} {label} {time}")
for label, time in event_records:
    print(f"SELF_SUSPEND_EVENT_REFERENCE {mode} {label} {time}")
