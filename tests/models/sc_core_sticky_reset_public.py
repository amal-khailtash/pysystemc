"""Compare sticky synchronous reset and descendant propagation with native threads."""

from amal.eda.systemc import sc_core as sc
import sys

include_children = sys.argv[1] == "descendants"
records = []
child_handle = None


def record(kind, label):
    """Record exact simulation time for native-comparable lifecycle ordering."""
    records.append(f"STICKY_RESET {kind} {label} {sc.sc_time_stamp().value()}")


def child():
    """Restart after each sticky-reset wake, then return once reset is disabled."""
    record("entry", "child")
    try:
        sc.wait(1, sc.SC_NS)
        record("done", "child")
    finally:
        record("cleanup", "child")


def parent():
    """Spawn a retained child once and execute cleanup on reset or normal return."""
    global child_handle
    record("entry", "parent")
    if include_children and child_handle is None:
        child_handle = sc.sc_spawn(child, "child")
    try:
        sc.wait(1, sc.SC_NS)
        record("done", "parent")
    finally:
        record("cleanup", "parent")


parent_handle = sc.sc_spawn(parent, "parent")


def controller():
    """Enable resets for two scheduled resumes, then restore ordinary execution."""
    sc.wait(0.25, sc.SC_NS)
    if include_children:
        parent_handle.sync_reset_on(sc.SC_INCLUDE_DESCENDANTS)
    else:
        parent_handle.sync_reset_on()
    sc.wait(2, sc.SC_NS)
    if include_children:
        parent_handle.sync_reset_off(sc.SC_INCLUDE_DESCENDANTS)
    else:
        parent_handle.sync_reset_off()


sc.sc_spawn(controller, "controller")
sc.sc_start(4, sc.SC_NS)
assert parent_handle.terminated()
assert child_handle is None or child_handle.terminated()
for label in ("parent", "child") if include_children else ("parent",):
    assert [line for line in records if line.startswith(f"STICKY_RESET entry {label}")] == [
        f"STICKY_RESET entry {label} {time}" for time in (0, 1000, 2000)
    ]
    assert [line for line in records if line.startswith(f"STICKY_RESET done {label}")] == [
        f"STICKY_RESET done {label} 3000"
    ]
    assert [line for line in records if line.startswith(f"STICKY_RESET cleanup {label}")] == [
        f"STICKY_RESET cleanup {label} {time}" for time in (1000, 2000, 3000)
    ]
print("\n".join(records))
sc.sc_stop()
