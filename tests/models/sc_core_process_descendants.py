"""Compare descendant process controls and cleanup against native SystemC."""

import sys

from amal.eda.systemc import sc_core as sc


mode = sys.argv[1]
if mode not in {"no_descendants", "include_descendants"}:
    raise ValueError(f"unsupported descendant mode: {mode}")
policy = sc.SC_NO_DESCENDANTS if mode == "no_descendants" else sc.SC_INCLUDE_DESCENDANTS
module_name = sc.sc_module_name("process_descendants")
module = sc.sc_module(module_name)
event = sc.sc_event("shared_event")
root_records = []
child_records = []
child_handle = None


def child_process():
    """Record each shared-event activation and final kill cleanup."""
    activation = 0
    child_records.append(("start", str(sc.sc_time_stamp())))
    try:
        while True:
            sc.wait(event)
            activation += 1
            child_records.append(("event", activation, str(sc.sc_time_stamp())))
    finally:
        child_records.append(("cleanup", str(sc.sc_time_stamp())))


def root_process():
    """Spawn a child process, then wait on the same event as its parent."""
    global child_handle
    activation = 0
    root_records.append(("start", str(sc.sc_time_stamp())))
    child_handle = sc.sc_spawn(child_process, "child")
    try:
        while True:
            sc.wait(event)
            activation += 1
            root_records.append(("event", activation, str(sc.sc_time_stamp())))
    finally:
        root_records.append(("cleanup", str(sc.sc_time_stamp())))


root_handle = sc.sc_spawn(root_process, "root")


def controller():
    """Exercise descendant policy at each operation before killing the tree."""
    sc.wait(1, sc.SC_NS)
    root_handle.disable(policy)
    event.notify(sc.SC_ZERO_TIME)
    sc.wait(1, sc.SC_NS)
    root_handle.enable(policy)
    event.notify(sc.SC_ZERO_TIME)
    sc.wait(1, sc.SC_NS)
    root_handle.suspend(policy)
    event.notify(sc.SC_ZERO_TIME)
    sc.wait(1, sc.SC_NS)
    root_handle.resume(policy)
    sc.wait(1, sc.SC_NS)
    root_handle.kill(policy)
    if mode == "no_descendants":
        assert not child_handle.terminated()
        child_handle.kill(sc.SC_NO_DESCENDANTS)


controller_handle = sc.sc_spawn(controller, "controller")
del module_name
sc.sc_start(6, sc.SC_NS)

assert root_handle.terminated()
assert child_handle.terminated()
assert controller_handle.terminated()
expected_root = [
    ("start", "0 s"),
    ("event", 1, "2 ns"),
    ("event", 2, "4 ns"),
    ("cleanup", "5 ns"),
]
expected_child = (
    [("start", "0 s"), ("event", 1, "1 ns"), ("event", 2, "2 ns"), ("event", 3, "3 ns"), ("cleanup", "5 ns")]
    if mode == "no_descendants"
    else [("start", "0 s"), ("event", 1, "2 ns"), ("event", 2, "4 ns"), ("cleanup", "5 ns")]
)
assert root_records == expected_root, (mode, root_records)
assert child_records == expected_child, (mode, child_records)
sc.sc_stop()
for record in root_records:
    print(f"DESCENDANT_REFERENCE {mode} root " + " ".join(map(str, record)))
for record in child_records:
    print(f"DESCENDANT_REFERENCE {mode} child " + " ".join(map(str, record)))
