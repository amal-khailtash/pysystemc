"""Probe lifecycle failure metadata and kernel state through native entry."""

import sys

from amal.eda.systemc import main, sc_core as sc


TARGET = sys.argv[1]
HOOKS = (
    "before_end_of_elaboration",
    "end_of_elaboration",
    "start_of_simulation",
    "end_of_simulation",
)
assert TARGET in HOOKS, TARGET
phases = []
processes = []
failure = None
models = []


class LifecycleModel(sc.sc_module):
    """Record native phase order and fail at one selected lifecycle hook."""

    def __init__(self, name):
        """Register an event-sensitive method to observe permitted execution."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.event = sc.sc_event("phase_event")
        self.sc_method("process", self.process)
        self.sensitive(self.event)
        self.dont_initialize()

    def phase(self, name):
        """Record a phase and raise only at the selected target."""
        phases.append(name)
        if TARGET == name:
            raise ValueError(f"lifecycle_metadata_marker:{name}")

    def process(self):
        """Record any scheduled process callback and its native timestamp."""
        processes.append((sc.sc_time_stamp().value(), sc.sc_delta_count()))

    def before_end_of_elaboration(self):
        """Record and optionally fail during the first elaboration hook."""
        self.phase("before_end_of_elaboration")

    def end_of_elaboration(self):
        """Record and optionally fail after elaboration completes."""
        self.phase("end_of_elaboration")

    def start_of_simulation(self):
        """Record and optionally fail as simulation starts."""
        self.phase("start_of_simulation")

    def end_of_simulation(self):
        """Record and optionally fail at simulation shutdown."""
        self.phase("end_of_simulation")


def sc_main(_args):
    """Run one failing lifecycle boundary and report native observations."""
    global failure
    model = LifecycleModel("lifecycle_metadata")
    models.append(model)
    model.event.notify(sc.SC_ZERO_TIME)
    assert sc.sc_get_status() == sc.SC_ELABORATION
    assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
    try:
        sc.sc_start(1, sc.SC_NS)
    except Exception as error:
        failure = ("start", type(error), str(error))
    after_start = (sc.sc_get_status(), sc.sc_time_stamp().value())
    stop_repeat_count = None
    if TARGET == "end_of_simulation":
        report_id = sc.SC_ID_SIMULATION_STOP_CALLED_TWICE_
        previous_actions = sc.sc_report_handler.set_actions(report_id, sc.SC_WARNING, sc.SC_THROW)
        try:
            sc.sc_stop()
        except Exception as error:
            assert failure is None
            failure = ("stop", type(error), str(error))
        stop_repeat_count = sc.sc_report_handler.get_count(report_id)
        try:
            sc.sc_stop()
        except sc.sc_report as error:
            raise AssertionError("repeated sc_stop emitted W545") from error
        assert sc.sc_report_handler.get_count(report_id) == stop_repeat_count
        sc.sc_report_handler.set_actions(report_id, sc.SC_WARNING, previous_actions)
    final = (sc.sc_get_status(), sc.sc_time_stamp().value())

    expected_phases = HOOKS[: HOOKS.index(TARGET) + 1]
    assert tuple(phases) == expected_phases, phases
    expected_processes = [(0, 0)] if TARGET == "end_of_simulation" else []
    assert processes == expected_processes, processes
    expected_stage = "stop" if TARGET == "end_of_simulation" else "start"
    assert failure is not None and failure[0] == expected_stage, failure
    assert failure[1] is ValueError, failure
    assert f"lifecycle_metadata_marker:{TARGET}" in failure[2], failure

    if TARGET == "before_end_of_elaboration":
        expected_after = (sc.SC_BEFORE_END_OF_ELABORATION, 0)
        expected_final = expected_after
    elif TARGET == "end_of_elaboration":
        expected_after = (sc.SC_END_OF_ELABORATION, 0)
        expected_final = expected_after
    elif TARGET == "start_of_simulation":
        expected_after = (sc.SC_START_OF_SIMULATION, 0)
        expected_final = expected_after
    else:
        expected_after = (sc.SC_PAUSED, sc.sc_time(1, sc.SC_NS).value())
        expected_final = (sc.SC_END_OF_SIMULATION, sc.sc_time(1, sc.SC_NS).value())
    assert after_start == expected_after, after_start
    assert final == expected_final, final
    phase_text = ",".join(phases)
    process_text = ",".join(f"{time}/{delta}" for time, delta in processes) or "none"
    print(
        "LIFECYCLE_ERROR_METADATA",
        TARGET,
        f"hooks={phase_text}",
        f"process={process_text}",
        f"error={failure[1].__name__}",
        f"diagnostic=lifecycle_metadata_marker:{TARGET}",
        f"after={int(after_start[0])}/{after_start[1]}",
        f"final={int(final[0])}/{final[1]}",
        f"stop_repeat_reports={stop_repeat_count if stop_repeat_count is not None else 'n/a'}",
    )
    return 0


main()
