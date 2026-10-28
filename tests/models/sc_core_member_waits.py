"""Compare module/channel member waits and scheduler unwind with native C++."""

import sys

from amal.eda.systemc import sc_core as sc
import cppyy

owner_kind, mode = sys.argv[1:]
name = sc.sc_module_name("top")
top = sc.sc_module(name)
channel = sc.sc_prim_channel("channel")
owner = cppyy.gbl.sc_core if owner_kind == "namespace" else top if owner_kind == "module" else channel
first, second = sc.sc_event("first"), sc.sc_event("second")
kinds = (
    "static",
    "count",
    "time",
    "units",
    "event",
    "or",
    "and",
    "time_event",
    "time_or",
    "time_and",
    "units_event",
    "units_or",
    "units_and",
)
records, starts, cleanups = [], {}, {}


def suspend(kind):
    """Call the familiar native member signature with borrowed sensitivity."""
    duration = sc.sc_time(3 if mode == "normal" else 100, sc.SC_NS)
    arguments = {
        "static": (),
        "count": (2,),
        "time": (duration,),
        "units": (3 if mode == "normal" else 100, sc.SC_NS),
        "event": (first,),
        "or": (first | second,),
        "and": (first & second,),
        "time_event": (duration, first),
        "time_or": (duration, first | second),
        "time_and": (duration, first & second),
        "units_event": (100, sc.SC_NS, first),
        "units_or": (100, sc.SC_NS, first | second),
        "units_and": (100, sc.SC_NS, first & second),
    }
    owner.wait(*arguments[kind])


def worker(kind):
    """Record completion or exactly one cleanup per interrupted invocation."""
    starts[kind] = starts.get(kind, 0) + 1
    try:
        suspend(kind)
        assert mode == "normal", "interrupted member wait returned"
        records.append(f"MEMBER_WAIT {kind} {sc.sc_time_stamp()}")
    finally:
        cleanups[kind] = cleanups.get(kind, 0) + 1
        if mode != "normal":
            records.append(f"MEMBER_CLEANUP {kind} {sc.sc_time_stamp()}")


handles = []
for kind in kinds:
    options = sc.sc_spawn_options()
    options.set_sensitivity(first)
    handles.append(sc.sc_spawn(lambda kind=kind: worker(kind), kind, options))
del name


def control():
    """Drive normal sensitivity or reset and kill every blocked process."""
    if mode == "normal":
        sc.wait(1, sc.SC_NS)
        first.notify(sc.SC_ZERO_TIME)
        sc.wait(1, sc.SC_NS)
        first.notify(sc.SC_ZERO_TIME)
        second.notify(sc.SC_ZERO_TIME)
    else:
        sc.wait(2, sc.SC_NS)
        for handle in handles:
            handle.reset()
        sc.wait(1, sc.SC_NS)
        for handle in handles:
            handle.kill()


controller = sc.sc_spawn(control)
sc.sc_start(4, sc.SC_NS)
assert all(h.terminated() for h in (*handles, controller))
assert starts == cleanups == dict.fromkeys(kinds, 1 if mode == "normal" else 2)
expected_times = {
    kind: "3 ns"
    if kind in ("time", "units")
    else "2 ns"
    if kind in ("count", "and", "time_and", "units_and")
    else "1 ns"
    for kind in kinds
}
if mode == "normal":
    assert sorted(records) == sorted(f"MEMBER_WAIT {kind} {time}" for kind, time in expected_times.items())
else:
    assert sorted(records) == sorted(f"MEMBER_CLEANUP {kind} {time} ns" for kind in kinds for time in (2, 3))
sc.sc_stop()
for line in sorted(records):
    print(line)
print("MEMBER_WAIT_OK", owner_kind, mode, len(kinds))
