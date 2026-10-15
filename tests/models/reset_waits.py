"""Stress asynchronous reset and kill across every native wait family."""

from amal.eda.systemc import sc_core as sc

clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
reset = sc.sc_signal[bool]("reset")
first = sc.sc_event("first")
second = sc.sc_event("second")
records = {}
cleanups = {}
handles = []


def worker(kind):
    """Restart on reset and execute cleanup exactly once per invocation."""
    records[kind] = records.get(kind, 0) + 1
    try:
        while True:
            if kind == "cycle":
                sc.wait()
            elif kind == "count":
                sc.wait(2)
            elif kind == "time":
                sc.wait(sc.sc_time(100, sc.SC_NS))
            elif kind == "units":
                sc.wait(100, sc.SC_NS)
            elif kind == "event":
                sc.wait(first)
            elif kind == "or":
                sc.wait(first | second)
            elif kind == "and":
                sc.wait(first & second)
            elif kind == "timed_or":
                sc.wait(sc.sc_time(100, sc.SC_NS), first | second)
            elif kind == "timed_and":
                sc.wait(100, sc.SC_NS, first & second)
            elif kind == "timed_event":
                sc.wait(sc.sc_time(100, sc.SC_NS), first)
    finally:
        cleanups[kind] = cleanups.get(kind, 0) + 1


for kind in ("cycle", "count", "time", "units", "event", "or", "and", "timed_or", "timed_and", "timed_event"):
    options = sc.sc_spawn_options()
    options.set_sensitivity(clock.posedge_event())
    options.async_reset_signal_is(reset, True)
    handles.append(sc.sc_spawn(lambda kind=kind: worker(kind), kind, options))


def controller():
    """Drive repeated async reset, explicit reset and termination from scheduler context."""
    for _ in range(8):
        sc.wait(3, sc.SC_NS)
        reset.write(True)
        sc.wait(sc.SC_ZERO_TIME)
        reset.write(False)
        sc.wait(sc.SC_ZERO_TIME)
    for handle in handles:
        handle.reset()
    for handle in handles:
        handle.kill()
    assert all(handle.terminated() for handle in handles)
    sc.sc_stop()


controller_handle = sc.sc_spawn(controller, "controller")
sc.sc_start(100, sc.SC_NS)
assert all(value == 10 for value in records.values()), records
assert records == cleanups, (records, cleanups)
try:
    handles[0].kill()
except RuntimeError:
    pass
else:
    raise AssertionError("paused/stopped process control accepted")
print("RESET_WAITS_OK")
