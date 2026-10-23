"""Compare the public ``sc_start`` overload family against native SystemC."""

from amal.eda.systemc import sc_core as sc
import cppyy

assert cppyy.gbl.sc_core.sc_start

_LONG_RUNS = {"time_run", "int_run", "double_run", "noarg"}
_ERROR = "error"
_callbacks = []
_scenario = ""
_tick = None
_boundary = None


def _record_callback():
    """Record the current kernel time and delta count."""
    now = sc.sc_time_stamp().value()
    delta = sc.sc_delta_count()
    _callbacks.append((now, delta))


def _first_callback():
    """Record the first event and schedule the boundary callback when required."""
    _record_callback()
    if _scenario == _ERROR:
        raise RuntimeError("callback thrown")
    if _scenario in _LONG_RUNS:
        _boundary.notify(sc.sc_time(2, sc.SC_NS))


def _boundary_callback():
    """Record an event exactly at the requested limit and stop the noarg run."""
    _record_callback()
    if _scenario == "noarg":
        sc.sc_stop()


def _expected_callbacks(scenario):
    """Return exact callback timestamps and delta counts for one scenario."""
    if scenario in _LONG_RUNS:
        return [(1000, 0), (3000, 1)]
    return [(1000, 0)]


def _run_error():
    """Check that a callback report keeps its first overload's report fields."""
    global _scenario, _tick, _boundary
    _scenario = _ERROR
    name = sc.sc_module_name("start_error")
    module = sc.sc_module(name)
    _tick = sc.sc_event("tick")
    _boundary = sc.sc_event("boundary")
    module.sc_method("callback", _first_callback)
    module.sensitive(_tick)
    module.dont_initialize()
    module.sc_method("boundary_callback", _boundary_callback)
    module.sensitive(_boundary)
    module.dont_initialize()
    del name
    sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_THROW)
    _tick.notify(sc.sc_time(1, sc.SC_NS))
    try:
        sc.sc_start(3.0, sc.SC_NS, sc.SC_RUN_TO_TIME)
    except sc.sc_report as report:
        message = report.get_msg()
        assert "callback thrown" in message
        assert "E548" not in message
        report_id = report.get_id()
        report_type = report.get_msg_type()
        severity = report.get_severity()
        assert report_id == 549
        assert report_type == "uncaught exception"
        assert severity == sc.SC_ERROR
        assert _callbacks == [(1000, 0)]
        print(
            "START_ERROR",
            report_id,
            report_type,
            int(severity),
            _callbacks[0][0],
            _callbacks[0][1],
            sep="\t",
        )
    else:
        raise AssertionError("callback report did not propagate from sc_start")


def run(scenario):
    """Run one overload and emit native-comparable callback and final state.

    Args:
        scenario: Overload/policy row selected by the subprocess test.

    Returns:
        None; prints normalized callback, exception, or final-state evidence.

    Raises:
        AssertionError: Native callback order, time, delta, status, or report fields differ.
    """
    global _scenario, _tick, _boundary
    _scenario = scenario
    if scenario == _ERROR:
        _run_error()
        return

    name = sc.sc_module_name("start_model")
    module = sc.sc_module(name)
    _tick = sc.sc_event("tick")
    _boundary = sc.sc_event("boundary")
    module.sc_method("callback", _first_callback)
    module.sensitive(_tick)
    module.dont_initialize()
    module.sc_method("boundary_callback", _boundary_callback)
    module.sensitive(_boundary)
    module.dont_initialize()
    del name
    _tick.notify(sc.sc_time(1, sc.SC_NS))

    if scenario == "noarg":
        sc.sc_start()
    else:
        policy = sc.SC_RUN_TO_TIME if scenario.endswith("run") else sc.SC_EXIT_ON_STARVATION
        if scenario.startswith("time_"):
            sc.sc_start(sc.sc_time(3, sc.SC_NS), policy)
        elif scenario.startswith("int_"):
            sc.sc_start(3, sc.SC_NS, policy)
        elif scenario.startswith("double_"):
            sc.sc_start(3.0, sc.SC_NS, policy)
        else:
            raise AssertionError(f"unknown scenario: {scenario}")

    expected_times = _expected_callbacks(scenario)
    if scenario.endswith("run"):
        assert _callbacks == [(1000, 0)]
        assert sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
        sc.sc_start(sc.SC_ZERO_TIME)
    assert _callbacks == expected_times, (scenario, _callbacks)
    if scenario.endswith("run"):
        expected_final = (3000, 2, int(sc.SC_PAUSED))
    elif scenario.endswith("exit"):
        expected_final = (1000, 1, int(sc.SC_PAUSED))
    else:
        expected_final = (3000, 1, int(sc.SC_STOPPED))
    final = (sc.sc_time_stamp().value(), sc.sc_delta_count(), int(sc.sc_get_status()))
    assert final == expected_final
    for index, (time, delta) in enumerate(_callbacks):
        print("START_CALLBACK", index, time, delta, sep="\t")
    print("START_FINAL", sc.sc_time_stamp().value(), sc.sc_delta_count(), int(sc.sc_get_status()), sep="\t")


if __name__ == "__main__":
    import sys

    run(sys.argv[1])
