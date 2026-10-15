"""Validate current-source event-list and wait-count diagnostics and termination."""

from amal.eda.systemc import sc_core as sc

reports = []


event = sc.sc_event("late_event")
empty_or = sc.sc_event_or_list()
empty_and = sc.sc_event_and_list()
assert empty_or.empty() and empty_and.empty()
assert empty_or.size() == 0 and empty_and.size() == 0
seen = []
reset = sc.sc_signal[bool]("initial_reset", True)
clock = sc.sc_clock("reset_clock", sc.sc_time(1, sc.SC_NS))
reset_entries = []


def reset_worker():
    """Record startup reset and suspend on the clock until killed."""
    reset_entries.append(str(sc.sc_time_stamp()))
    while True:
        sc.wait(2)


options = sc.sc_spawn_options()
options.set_sensitivity(clock.posedge_event())
options.async_reset_signal_is(reset, True)
reset_handle = sc.sc_spawn(reset_worker, "initial_reset_worker", options)


def reset_controller():
    """Release startup reset, restart during a counted wait and terminate."""
    sc.wait(0.25, sc.SC_NS)
    reset.write(False)
    sc.wait(1, sc.SC_NS)
    reset.write(True)
    sc.wait(sc.SC_ZERO_TIME)
    reset.write(False)
    sc.wait(0.25, sc.SC_NS)
    reset_handle.kill()


sc.sc_spawn(reset_controller, "reset_controller")


def invalid_waits():
    """Issue invalid counts and empty waits, then terminate normally."""
    for value in (0, -1, empty_or, empty_and):
        try:
            sc.wait(value)
        except Exception as error:
            reports.append(str(error))
        else:
            raise AssertionError("invalid wait accepted")
    seen.append("returned")


def terminated():
    """Wait dynamically once and finish before another event notification."""
    sc.wait(event)
    seen.append("terminated")


invalid = sc.sc_spawn(invalid_waits, "invalid")
handle = sc.sc_spawn(terminated, "terminated")
sc.sc_start(1, sc.SC_NS)
event.notify(sc.SC_ZERO_TIME)
sc.sc_start(1, sc.SC_NS)
assert handle.terminated()
event.notify(sc.SC_ZERO_TIME)
sc.sc_start(1, sc.SC_NS)
assert seen.count("terminated") == 1, seen
assert invalid.terminated(), seen
assert len(reports) == 4, reports
assert reset_entries[0] == "0 s", reset_entries
assert "1250 ps" in reset_entries, reset_entries
assert reset_handle.terminated()
sc.sc_stop()
print("SOURCE_CORNER_CASES_OK")
