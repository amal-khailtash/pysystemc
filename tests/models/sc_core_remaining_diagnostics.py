"""Compare remaining recoverable kernel diagnostics with native SystemC."""

import sys

from amal.eda.systemc import sc_core as sc
import cppyy


SCENARIOS = {
    "orphan",
    "idle_start",
    "overflow",
    "unwinding",
    "self_notify",
    "module_warnings",
    "module_stack",
    "after_error",
    "raw_controls",
    "nested_start",
    "cycle_misses",
    "phase_callback_forbidden",
    "module_binding_error",
}
scenario = sys.argv[1]
assert scenario in SCENARIOS, scenario
handler = sc.sc_report_handler
rows = []
current_label = ""


def normalized(value):
    """Collapse native diagnostic whitespace for portable parity records."""
    return " ".join(str(value).split())


def capture(report, actions):
    """Copy native report metadata then preserve native report actions."""
    if current_label and int(report.get_severity()) != int(sc.SC_INFO):
        label = current_label
        if label == "w509_missing_end" and report.get_id() == 510:
            label = "w510_hierarchy"
        rows.append(
            (
                label,
                int(report.get_id()),
                int(report.get_severity()),
                str(report.get_msg_type()),
                normalized(report.get_msg()),
                int(report.get_time().value()),
                str(report.get_process_name() or ""),
                int(report.get_verbosity()),
                int(actions),
            )
        )
    handler.default_handler(report, actions)


sc.set_report_handler(capture)


def attempt(label, expected_id, operation):
    """Require one throwing report and verify its metadata and kernel time."""
    global current_label
    current_label = label
    before = len(rows)
    try:
        operation()
    except sc.sc_report as report:
        assert len(rows) == before + 1, (label, rows[before:])
        row = rows[-1]
        assert row[0] == label and row[1] == expected_id, row
        assert row[2] == int(sc.SC_ERROR), row
        assert row[3] == report.get_msg_type(), row
        assert row[5] == int(sc.sc_time_stamp().value()), (row, sc.sc_time_stamp())
        assert report.get_id() == expected_id
        return
    raise AssertionError(f"{label}: configured native error did not throw")


def records():
    """Print copied reports and a scenario completion marker for native parity."""
    for row in rows:
        print("REMAINING_DIAGNOSTIC_REPORT", scenario, *row, sep="|")
    print(
        "REMAINING_DIAGNOSTIC_STATE",
        scenario,
        sc.sc_time_stamp().value(),
        sc.sc_delta_count(),
        int(sc.sc_get_status()),
        sep="|",
    )


def set_actions(report_id, severity, actions):
    """Select a deterministic native action and return the prior policy."""
    return handler.set_actions(report_id, severity, actions)


def run_orphan():
    """Report W558 for a no-sensitivity method suppressed from initialization."""
    global current_label
    old_actions = handler.set_actions(sc.SC_WARNING, sc.SC_DO_NOTHING)
    name = sc.sc_module_name("orphan_owner")
    module = sc.sc_module(name)
    called = []
    module.sc_method("orphan", lambda: called.append("ran"))
    module.dont_initialize()
    del name
    current_label = "w558_orphan"
    sc.sc_start(sc.SC_ZERO_TIME)
    current_label = ""
    assert called == [], called
    assert len(rows) == 1 and rows[0][1:4] == (
        558,
        int(sc.SC_WARNING),
        "disable() or dont_initialize() called on process with no static sensitivity, it will be orphaned",
    ), rows
    assert sc.sc_time_stamp().value() == 0
    handler.set_actions(sc.SC_WARNING, old_actions)


def run_idle_start():
    """Report W571 only on a later inactive zero-time start."""
    global current_label
    old_actions = handler.set_actions(sc.SC_WARNING, sc.SC_DO_NOTHING)
    sc.sc_start(sc.SC_ZERO_TIME)
    assert not rows
    current_label = "w571_idle"
    sc.sc_start(sc.SC_ZERO_TIME)
    current_label = ""
    assert len(rows) == 1 and rows[0][1:4] == (
        571,
        int(sc.SC_WARNING),
        "no activity or clock movement for sc_start() invocation",
    ), rows
    assert sc.sc_time_stamp().value() == 0
    handler.set_actions(sc.SC_WARNING, old_actions)


def run_overflow():
    """Catch E544 before it changes time after a finite initial advance."""
    sc.sc_start(1, sc.SC_PS, sc.SC_RUN_TO_TIME)
    before = (int(sc.sc_time_stamp().value()), int(sc.sc_delta_count()), int(sc.sc_get_status()))
    old = set_actions(sc.SC_ID_SIMULATION_TIME_OVERFLOW_, sc.SC_ERROR, sc.SC_THROW)
    attempt("e544_overflow", 544, lambda: sc.sc_start(sc.sc_max_time(), sc.SC_RUN_TO_TIME))
    after = (int(sc.sc_time_stamp().value()), int(sc.sc_delta_count()), int(sc.sc_get_status()))
    assert after == before, (before, after)
    handler.set_actions(sc.SC_ID_SIMULATION_TIME_OVERFLOW_, sc.SC_ERROR, old)


def run_unwinding():
    """Reenter reset from a target's Python finally block during reset unwind."""
    global current_label
    old_actions = handler.set_actions(sc.SC_WARNING, sc.SC_DO_NOTHING)
    owner_name = sc.sc_module_name("unwind_owner")
    owner = sc.sc_module(owner_name)
    target = {"handle": None}
    entries = []
    cleanup = []
    controller_done = []

    def worker():
        """Block once, then reenter reset from Python cleanup while unwinding."""
        global current_label
        entries.append(int(sc.sc_time_stamp().value()))
        try:
            sc.wait(20, sc.SC_NS)
        finally:
            cleanup.append(int(sc.sc_time_stamp().value()))
            if len(entries) == 1:
                current_label = "w540_reentrant_reset"
                target["handle"].reset()
                current_label = ""

    def controller():
        """Reset the target once, then stop after it restarts."""
        global current_label
        sc.wait(1, sc.SC_NS)
        target["handle"].reset()
        sc.wait(1, sc.SC_NS)
        controller_done.append(int(sc.sc_time_stamp().value()))
        sc.sc_stop()

    owner.sc_thread("target", worker)
    target["handle"] = sc.sc_get_last_created_process_handle()
    owner.sc_thread("controller", controller)
    del owner_name
    sc.sc_start(5, sc.SC_NS, sc.SC_RUN_TO_TIME)
    current_label = ""
    assert rows and rows[0][1:4] == (
        540,
        int(sc.SC_WARNING),
        "kill/reset ignored during unwinding",
    ), rows
    assert len(rows) == 1, rows
    assert entries == [0, 1_000], entries
    assert cleanup == [1_000], cleanup
    assert controller_done == [2_000], controller_done
    assert not target["handle"].terminated()
    assert sc.sc_time_stamp().value() == 2_000
    handler.set_actions(sc.SC_WARNING, old_actions)


def run_self_notify():
    """Report W536 when a running method immediately notifies itself."""
    global current_label
    old_actions = handler.set_actions(sc.SC_WARNING, sc.SC_DO_NOTHING)
    owner_name = sc.sc_module_name("self_notify_owner")
    owner = sc.sc_module(owner_name)
    event = sc.sc_event("self_notify_event")
    calls = []

    def notify_self():
        """Send one immediate event while this method is the current process."""
        global current_label
        calls.append(int(sc.sc_time_stamp().value()))
        current_label = "w536_self_notify"
        event.notify()
        current_label = ""

    owner.sc_method("notify_self", notify_self)
    owner.sensitive(event)
    owner.dont_initialize()
    del owner_name
    event.notify(sc.sc_time(1, sc.SC_NS))
    sc.sc_start(2, sc.SC_NS, sc.SC_RUN_TO_TIME)
    assert len(rows) == 1 and rows[0][1:4] == (
        536,
        int(sc.SC_WARNING),
        "immediate self-notification ignored as of IEEE 1666-2011",
    ), rows
    assert calls == [1_000], calls
    assert sc.sc_time_stamp().value() == 2_000
    handler.set_actions(sc.SC_WARNING, old_actions)


def run_module_warnings():
    """Compare legacy constructor and incomplete module warnings W569/W509/W510."""
    global current_label
    old_actions = handler.set_actions(sc.SC_WARNING, sc.SC_DO_NOTHING)

    cppyy.cppdef(
        """
        #include <systemc>
        namespace pysystemc_f4_remaining {
        class LegacyModule : public sc_core::sc_module {
        public: explicit LegacyModule(const char* name) : sc_module(name) {}
        };
        }
        """
    )
    LegacyModule = cppyy.gbl.pysystemc_f4_remaining.LegacyModule

    current_label = "w569_legacy_one"
    first = LegacyModule("legacy_one")
    current_label = "w569_legacy_two"
    second = LegacyModule("legacy_two")
    current_label = "w509_missing_end"
    sc.sc_start(sc.SC_ZERO_TIME)
    current_label = ""
    expected = [
        ("w569_legacy_one", 569, int(sc.SC_WARNING), "legacy_one"),
        ("w569_legacy_two", 569, int(sc.SC_WARNING), "legacy_two"),
        ("w509_missing_end", 509, int(sc.SC_WARNING), "module 'legacy_one'"),
        ("w509_missing_end", 509, int(sc.SC_WARNING), "module 'legacy_one.legacy_two'"),
        ("w510_hierarchy", 510, int(sc.SC_WARNING), ""),
    ]
    assert [(r[0], r[1], r[2], r[4]) for r in rows] == expected, rows
    assert sc.sc_get_status() == sc.SC_PAUSED
    assert sc.sc_time_stamp().value() == 0
    assert first.name() == "legacy_one" and second.name() == "legacy_one.legacy_two"
    handler.set_actions(sc.SC_WARNING, old_actions)


def run_module_stack():
    """Report W511 when setting stack size on the latest method process."""
    global current_label
    old_actions = handler.set_actions(sc.SC_WARNING, sc.SC_DO_NOTHING)
    name = sc.sc_module_name("stack_owner")
    module = sc.sc_module(name)
    module.sc_method("method", lambda: None)
    current_label = "w511_method_stack"
    module.set_stack_size(1024)
    current_label = ""
    assert len(rows) == 1 and rows[0][1:4] == (
        511,
        int(sc.SC_WARNING),
        "set_stack_size() is only allowed for SC_THREADs and SC_CTHREADs",
    ), rows
    assert module.name() == "stack_owner"
    handler.set_actions(sc.SC_WARNING, old_actions)
    del name


def run_after_error():
    """Reach E548 only after a process callback made sc_start enter error state."""
    global current_label
    cppyy.cppdef(
        """
        #include <stdexcept>
        #include <systemc>
        namespace pysystemc_f4_remaining {
        class ErrorModule : public sc_core::sc_module {
          SC_HAS_PROCESS(ErrorModule);
        public:
          explicit ErrorModule(sc_core::sc_module_name name) : sc_module(name) { SC_METHOD(fail); }
          void fail() { throw std::runtime_error("prior kernel error"); }
        };
        }
        """
    )
    name = sc.sc_module_name("prior_error_owner")
    _owner = cppyy.gbl.pysystemc_f4_remaining.ErrorModule(name)
    del name
    old_uncaught = set_actions(sc.SC_ID_SIMULATION_UNCAUGHT_EXCEPTION_, sc.SC_ERROR, sc.SC_THROW)
    sc.set_report_handler()
    try:
        sc.sc_start(2, sc.SC_NS, sc.SC_RUN_TO_TIME)
    except sc.sc_report as report:
        assert report.get_id() == 549, report.get_id()
    else:
        raise AssertionError("setup did not put the kernel in an error state")
    status_before = int(sc.sc_get_status())
    status_before_internal = int(sc.sc_get_curr_simcontext().sim_status())
    assert status_before_internal == int(sc.SC_SIM_ERROR), status_before_internal
    sc.set_report_handler(capture)
    old_after = set_actions(sc.SC_ID_SIMULATION_START_AFTER_ERROR_, sc.SC_ERROR, sc.SC_THROW)
    attempt("e548_restart_after_error", 548, lambda: sc.sc_start(sc.SC_ZERO_TIME))
    assert int(sc.sc_get_status()) == status_before, (status_before, sc.sc_get_status())
    assert int(sc.sc_get_curr_simcontext().sim_status()) == status_before_internal
    handler.set_actions(sc.SC_ID_SIMULATION_START_AFTER_ERROR_, sc.SC_ERROR, old_after)
    handler.set_actions(sc.SC_ID_SIMULATION_UNCAUGHT_EXCEPTION_, sc.SC_ERROR, old_uncaught)


def run_nested_start():
    """Catch E554 from a public sc_start call nested inside a method process."""
    global current_label
    old = set_actions(sc.SC_ID_SIMULATION_START_UNEXPECTED_, sc.SC_ERROR, sc.SC_THROW)
    name = sc.sc_module_name("nested_start_owner")
    owner = sc.sc_module(name)
    event = sc.sc_event("nested_start_event")
    callbacks = []

    def nested():
        """Record the outer callback and catch its reentrant start report."""
        callbacks.append(int(sc.sc_time_stamp().value()))
        attempt("e554_nested_start", 554, lambda: sc.sc_start(sc.SC_ZERO_TIME))

    owner.sc_method("nested", nested)
    owner.sensitive(event)
    owner.dont_initialize()
    del name
    event.notify(sc.sc_time(1, sc.SC_NS))
    sc.sc_start(2, sc.SC_NS, sc.SC_RUN_TO_TIME)
    assert callbacks == [1_000], callbacks
    assert sc.sc_time_stamp().value() == 2_000
    assert sc.sc_get_status() == sc.SC_PAUSED
    handler.set_actions(sc.SC_ID_SIMULATION_START_UNEXPECTED_, sc.SC_ERROR, old)


def run_cycle_misses():
    """Report W538 through the exported deprecated sc_cycle entry point."""
    global current_label
    cppyy.cppdef("namespace sc_core { void sc_cycle(const sc_time&); }")
    old_actions = handler.set_actions(sc.SC_WARNING, sc.SC_DO_NOTHING)
    name = sc.sc_module_name("cycle_owner")
    owner = sc.sc_module(name)
    event = sc.sc_event("cycle_pending_event")
    callbacks = []
    owner.sc_method("observe", lambda: callbacks.append(int(sc.sc_time_stamp().value())))
    owner.sensitive(event)
    owner.dont_initialize()
    del name
    sc.sc_start(sc.SC_ZERO_TIME)
    event.notify(sc.sc_time(1, sc.SC_NS))
    current_label = "w538_cycle_misses_events"
    cppyy.gbl.sc_core.sc_cycle(sc.sc_time(1, sc.SC_NS))
    current_label = ""
    assert len(rows) == 1 and rows[0][1:4] == (
        538,
        int(sc.SC_WARNING),
        sc.SC_ID_CYCLE_MISSES_EVENTS_,
    ), rows
    assert sc.sc_time_stamp().value() == 1_000 and not callbacks
    sc.sc_start(1, sc.SC_NS, sc.SC_RUN_TO_TIME)
    assert callbacks == [1_000], callbacks
    assert sc.sc_time_stamp().value() == 2_000
    handler.set_actions(sc.SC_WARNING, old_actions)


def run_phase_callback_forbidden():
    """Report W553 when a phase callback dynamically creates processes."""
    global current_label
    cppyy.cppdef(
        r"""
        #define SC_INCLUDE_DYNAMIC_PROCESSES
        #include <string>
        #include <systemc>
        namespace pysystemc_f4_remaining {
        class PhaseProbe : public sc_core::sc_object {
        public:
          explicit PhaseProbe(const char* name) : sc_object(name) {
            register_simulation_phase_callback(sc_core::SC_END_OF_UPDATE |
                                               sc_core::SC_BEFORE_TIMESTEP);
          }
          int update_calls = 0;
          int timestep_calls = 0;
        private:
          void simulation_phase_callback() override {
            const auto phase = sc_core::sc_get_status();
            if (phase == sc_core::SC_END_OF_UPDATE && update_calls++ == 0) {
              spawn_pair("end_update");
            } else if (phase == sc_core::SC_BEFORE_TIMESTEP && timestep_calls++ == 0) {
              spawn_pair("before_timestep");
            }
          }
          void spawn_pair(const char* phase) {
            const std::string method_name = std::string("method_") + phase;
            const std::string thread_name = std::string("thread_") + phase;
            sc_core::sc_spawn_options method_options;
            method_options.spawn_method();
            sc_core::sc_spawn([] {}, method_name.c_str(), &method_options);
            sc_core::sc_spawn([] { sc_core::wait(1, sc_core::SC_NS); }, thread_name.c_str());
          }
        };
        }
        """
    )
    probe = cppyy.gbl.pysystemc_f4_remaining.PhaseProbe("phase_probe")
    timer = sc.sc_event("phase_probe_timer")
    timer.notify(sc.sc_time(1, sc.SC_NS))
    current_label = "w553_phase_callback"
    sc.sc_start(2, sc.SC_NS, sc.SC_RUN_TO_TIME)
    current_label = ""
    assert [row[1] for row in rows] == [553, 553, 553, 553], rows
    assert all(row[2] == int(sc.SC_WARNING) for row in rows), rows
    assert int(probe.update_calls) >= 1 and int(probe.timestep_calls) >= 1
    assert sc.sc_time_stamp().value() == 2_000


def run_module_binding_error():
    """Catch E107 from an incompatible positional bind, then bind correctly."""
    global current_label
    cppyy.cppdef(
        r"""
        #include <systemc>
        namespace pysystemc_f4_remaining {
        /** @brief Module with one boolean input for positional bind recovery. */
        class BindModule : public sc_core::sc_module {
        public:
          explicit BindModule(sc_core::sc_module_name name) : sc_module(name), input("input") {}
          sc_core::sc_in<bool> input;
        };
        /** @brief Invoke the exact module-to-interface positional overload.
         * @param module Borrowed module whose positional port is under test.
         * @param interface_ Borrowed interface whose lifetime spans this call.
         * @details No pointer or callback is retained. The Python caller retains
         * the GIL; native SC_THROW reports propagate synchronously to it.
         */
        void positional_bind_interface(sc_core::sc_module& module, sc_core::sc_interface& interface_) {
          module << interface_;
        }
        /** @brief Return a borrowed native interface view without copying a signal.
         * @param signal Live native signal retained by the Python scenario.
         * @return A base-interface reference valid while the signal remains alive.
         */
        sc_core::sc_interface& as_interface(sc_core::sc_signal<int>& signal) { return signal; }
        /** @brief Return a borrowed native interface view of a boolean signal.
         * @param signal Live native signal retained by the Python scenario.
         * @return A base-interface reference valid while the signal remains alive.
         */
        sc_core::sc_interface& as_interface(sc_core::sc_signal<bool>& signal) { return signal; }
        }
        """
    )
    BindModule = cppyy.gbl.pysystemc_f4_remaining.BindModule
    positional_bind_interface = cppyy.gbl.pysystemc_f4_remaining.positional_bind_interface
    as_interface = cppyy.gbl.pysystemc_f4_remaining.as_interface
    name = sc.sc_module_name("bind_owner")
    owner = BindModule(name)
    del name
    wrong = sc.sc_signal[int]("wrong_signal")
    correct = sc.sc_signal[bool]("correct_signal")
    wrong_interface = as_interface(wrong)
    correct_interface = as_interface(correct)
    old = set_actions(sc.SC_ID_BIND_IF_TO_PORT_, sc.SC_ERROR, sc.SC_THROW)
    before = (int(sc.sc_time_stamp().value()), int(sc.sc_delta_count()), int(sc.sc_get_status()))
    attempt(
        "e107_module_binding_type_mismatch",
        107,
        lambda: positional_bind_interface(owner, wrong_interface),
    )
    assert len(rows) == 1, rows
    row = rows[-1]
    assert row[1:] == (
        107,
        int(sc.SC_ERROR),
        str(sc.SC_ID_BIND_IF_TO_PORT_),
        "type mismatch on port 0 of module `bind_owner'",
        before[0],
        "",
        int(sc.SC_MEDIUM),
        int(sc.SC_THROW),
    ), row
    assert (int(sc.sc_time_stamp().value()), int(sc.sc_delta_count()), int(sc.sc_get_status())) == before
    positional_bind_interface(owner, correct_interface)
    sc.sc_start(sc.SC_ZERO_TIME)
    assert not bool(owner.input.read())
    assert sc.sc_get_status() == sc.SC_PAUSED
    assert sc.sc_time_stamp().value() == 0
    handler.set_actions(sc.SC_ID_BIND_IF_TO_PORT_, sc.SC_ERROR, old)


def run_raw_controls():
    """Reach raw E572/E573 native methods through C++ and preserve the handle."""
    global current_label
    cppyy.cppdef(
        """
        #include <systemc>
        namespace pysystemc_f4_remaining {
        void raw_kill(sc_core::sc_process_handle& handle) { handle.kill(); }
        void raw_reset(sc_core::sc_process_handle& handle) { handle.reset(); }
        }
        """
    )
    name = sc.sc_module_name("raw_control_owner")
    owner = sc.sc_module(name)
    owner.sc_thread("target", lambda: sc.wait(10, sc.SC_NS))
    handle = sc.sc_get_last_created_process_handle()
    old_kill = set_actions(sc.SC_ID_KILL_PROCESS_WHILE_UNITIALIZED_, sc.SC_ERROR, sc.SC_THROW)
    old_reset = set_actions(sc.SC_ID_RESET_PROCESS_WHILE_NOT_RUNNING_, sc.SC_ERROR, sc.SC_THROW)
    attempt("e572_raw_kill_before_start", 572, lambda: cppyy.gbl.pysystemc_f4_remaining.raw_kill(handle))
    assert handle.valid() and handle.name() == "raw_control_owner.target" and not handle.terminated()
    attempt("e573_raw_reset_before_start", 573, lambda: cppyy.gbl.pysystemc_f4_remaining.raw_reset(handle))
    assert handle.valid() and handle.name() == "raw_control_owner.target" and not handle.terminated()
    assert sc.sc_get_status() == sc.SC_ELABORATION
    handler.set_actions(sc.SC_ID_KILL_PROCESS_WHILE_UNITIALIZED_, sc.SC_ERROR, old_kill)
    handler.set_actions(sc.SC_ID_RESET_PROCESS_WHILE_NOT_RUNNING_, sc.SC_ERROR, old_reset)
    del name


{
    "orphan": run_orphan,
    "idle_start": run_idle_start,
    "overflow": run_overflow,
    "unwinding": run_unwinding,
    "self_notify": run_self_notify,
    "module_warnings": run_module_warnings,
    "module_stack": run_module_stack,
    "after_error": run_after_error,
    "raw_controls": run_raw_controls,
    "nested_start": run_nested_start,
    "cycle_misses": run_cycle_misses,
    "phase_callback_forbidden": run_phase_callback_forbidden,
    "module_binding_error": run_module_binding_error,
}[scenario]()
records()
