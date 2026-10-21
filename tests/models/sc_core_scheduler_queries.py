"""Compare native pending-activity, delta and lifecycle queries at fixed times."""

from amal.eda.systemc import sc_core as sc

context = sc.sc_get_curr_simcontext()
event = sc.sc_event("future")


def snapshot(label):
    """Check default/explicit query agreement and emit native-comparable state."""
    queries = (
        sc.sc_pending_activity_at_current_time,
        sc.sc_pending_activity_at_future_time,
        sc.sc_pending_activity,
        sc.sc_time_to_pending_activity,
        sc.sc_is_running,
    )
    values = []
    for query in queries:
        value = query()
        assert value == query(context)
        values.append(value)
    assert values[2] == (values[0] or values[1])
    print(
        "SCHEDULER_REFERENCE",
        label,
        int(sc.sc_get_status()),
        sc.sc_delta_count(),
        sc.sc_delta_count_at_current_time(),
        int(sc.sc_start_of_simulation_invoked()),
        int(sc.sc_end_of_simulation_invoked()),
        *(int(v) for v in values[:3]),
        values[3].value(),
        int(values[4]),
        sc.sc_time_stamp().value(),
    )


snapshot("construction")
for query in (
    sc.sc_pending_activity,
    sc.sc_pending_activity_at_current_time,
    sc.sc_pending_activity_at_future_time,
    sc.sc_time_to_pending_activity,
):
    try:
        query(object())
    except TypeError:
        pass
    else:
        raise AssertionError("invalid context accepted")
sc.sc_start(sc.SC_ZERO_TIME)
snapshot("initialized")
event.notify(5, sc.SC_NS)
snapshot("scheduled")
sc.sc_start(2, sc.SC_NS, sc.SC_RUN_TO_TIME)
snapshot("two_ns")
sc.sc_start(3, sc.SC_NS, sc.SC_RUN_TO_TIME)
snapshot("five_ns")
sc.sc_start(sc.SC_ZERO_TIME)
snapshot("drained")
sc.sc_stop()
snapshot("stopped")
