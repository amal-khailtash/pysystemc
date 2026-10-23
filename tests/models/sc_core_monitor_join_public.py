"""Compare public process-monitor and join contracts to native SystemC."""

from amal.eda.systemc import sc_core as sc

assert int(sc.sc_process_monitor.spm_exit) == 0

join_records = []
cleanup_records = []
rejection_records = []


class Model(sc.sc_module):
    """Own dynamic and clocked workers and their process-monitor joins."""

    def __init__(self, name):
        """Register join members before simulation and retain all join objects.

        Args:
            name: Native module name.
        """
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.method_event = sc.sc_event("method_event")
        self.sc_method("method_target", self.method_target)
        self.method_handle = sc.sc_get_last_created_process_handle()
        self.sensitive(self.method_event)
        self.dont_initialize()

        self.rejection_join = sc.sc_join()
        self.verify_join_rejection("method", self.method_handle)
        self.verify_join_rejection("empty", sc.sc_process_handle())

        self.dynamic_join = sc.sc_join()
        self.sc_thread("dynamic_fast", self.dynamic_fast)
        self.dynamic_fast_handle = sc.sc_get_last_created_process_handle()
        self.dynamic_join.add_process(self.dynamic_fast_handle)
        self.sc_thread("dynamic_slow", self.dynamic_slow)
        self.dynamic_join.add_process(sc.sc_get_last_created_process_handle())
        self.sc_thread("dynamic_waiter", self.dynamic_waiter)
        self.sc_thread("late_monitor", self.late_monitor)

        self.clock = sc.sc_clock("join_clock", sc.sc_time(2, sc.SC_NS))
        self.clock_port = sc.sc_in[bool]("clock_port")
        self.clock_port.bind(self.clock)
        self.clock_join = sc.sc_join()
        self.sc_cthread("clocked_fast", self.clocked_fast, self.clock_port.pos())
        self.clock_join.add_process(sc.sc_get_last_created_process_handle())
        self.sc_cthread("clocked_slow", self.clocked_slow, self.clock_port.pos())
        self.clock_join.add_process(sc.sc_get_last_created_process_handle())
        self.sc_cthread("clocked_waiter", self.clocked_waiter, self.clock_port.pos())
        self.dynamic_control_join = sc.sc_join()
        self.sc_thread("dynamic_control_fast", self.dynamic_control_fast)
        self.dynamic_control_join.add_process(sc.sc_get_last_created_process_handle())
        self.sc_thread("dynamic_control_slow", self.dynamic_control_slow)
        self.dynamic_control_join.add_process(sc.sc_get_last_created_process_handle())
        self.sc_thread("dynamic_control_waiter", self.dynamic_control_waiter)
        self.dynamic_control_handle = sc.sc_get_last_created_process_handle()

        self.clocked_control_join = sc.sc_join()
        self.sc_cthread("clocked_control_fast", self.clocked_control_fast, self.clock_port.pos())
        self.clocked_control_join.add_process(sc.sc_get_last_created_process_handle())
        self.sc_cthread("clocked_control_slow", self.clocked_control_slow, self.clock_port.pos())
        self.clocked_control_join.add_process(sc.sc_get_last_created_process_handle())
        self.sc_cthread("clocked_control_waiter", self.clocked_control_waiter, self.clock_port.pos())
        self.clocked_control_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("control_waiters", self.control_waiters)
        self.sc_thread("stopper", self.stopper)

    def method_target(self):
        """Provide a valid method handle that sc_join must reject."""

    def verify_join_rejection(self, label, handle):
        """Capture the typed native error for a method or empty handle.

        Args:
            label: Stable record name for the rejected handle kind.
            handle: Native process handle passed to sc_join.add_process.
        """
        previous = sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_THROW)
        try:
            try:
                self.rejection_join.add_process(handle)
            except sc.sc_report as report:
                assert report.get_id() == 561
                assert report.get_severity() == sc.SC_ERROR
                assert report.get_msg_type() == sc.SC_ID_JOIN_ON_METHOD_HANDLE_
                assert self.rejection_join.process_count() == 0
                rejection_records.append(
                    (
                        label,
                        int(report.get_id()),
                        int(report.get_severity()),
                        report.get_msg_type(),
                        report.get_msg(),
                        int(self.rejection_join.process_count()),
                    )
                )
            else:
                raise AssertionError(f"sc_join.add_process accepted {label} handle")
        finally:
            sc.sc_report_handler.set_actions(sc.SC_ERROR, previous)

    def dynamic_fast(self):
        """End the first dynamic worker after one nanosecond."""
        sc.wait(1, sc.SC_NS)

    def dynamic_slow(self):
        """End the second dynamic worker after three nanoseconds."""
        sc.wait(3, sc.SC_NS)

    def dynamic_waiter(self):
        """Wait for both dynamic workers and record the completed join count."""
        join_records.append(("dynamic_start", int(self.dynamic_join.process_count()), int(sc.sc_time_stamp().value())))
        self.dynamic_join.wait()
        join_records.append(("dynamic_done", int(self.dynamic_join.process_count()), int(sc.sc_time_stamp().value())))

    def late_monitor(self):
        """Add a monitor to a process that has already terminated and observe its count."""
        sc.wait(2, sc.SC_NS)
        self.late_join = sc.sc_join()
        self.late_join.add_process(self.dynamic_fast_handle)
        join_records.append(("late_add", int(self.late_join.process_count()), int(sc.sc_time_stamp().value())))

    def clocked_fast(self):
        """Terminate after the next positive clock edge."""
        sc.wait()

    def clocked_slow(self):
        """Terminate after two positive clock edges."""
        sc.wait(2)

    def clocked_waiter(self):
        """Wait through clocked cycles until both CTHREAD workers exit."""
        join_records.append(("clocked_start", int(self.clock_join.process_count()), int(sc.sc_time_stamp().value())))
        self.clock_join.wait_clocked()
        join_records.append(("clocked_done", int(self.clock_join.process_count()), int(sc.sc_time_stamp().value())))

    def dynamic_control_fast(self):
        """Keep the reset and kill join alive until six nanoseconds."""
        sc.wait(6, sc.SC_NS)

    def dynamic_control_slow(self):
        """Keep the reset and kill join alive until eight nanoseconds."""
        sc.wait(8, sc.SC_NS)

    def dynamic_control_waiter(self):
        """Record cleanup around reset and kill of a dynamic join wait."""
        cleanup_records.append(("dynamic_start", int(sc.sc_time_stamp().value())))
        try:
            self.dynamic_control_join.wait()
        finally:
            cleanup_records.append(("dynamic_cleanup", int(sc.sc_time_stamp().value())))

    def clocked_control_fast(self):
        """Remain monitored for three positive edges."""
        sc.wait(3)

    def clocked_control_slow(self):
        """Remain monitored for four positive edges."""
        sc.wait(4)

    def clocked_control_waiter(self):
        """Record cleanup around reset and kill of a clocked join wait."""
        cleanup_records.append(("clocked_start", int(sc.sc_time_stamp().value())))
        try:
            self.clocked_control_join.wait_clocked()
        finally:
            cleanup_records.append(("clocked_cleanup", int(sc.sc_time_stamp().value())))

    def control_waiters(self):
        """Reset and kill each waiter while retaining both joins for worker exit."""
        sc.wait(500, sc.SC_PS)
        self.dynamic_control_handle.reset()
        self.clocked_control_handle.reset()
        sc.wait(500, sc.SC_PS)
        self.dynamic_control_handle.kill()
        sc.wait(1500, sc.SC_PS)
        self.clocked_control_handle.kill()
        sc.wait(8, sc.SC_NS)
        assert int(self.dynamic_control_join.process_count()) == 0
        assert int(self.clocked_control_join.process_count()) == 0
        sc.sc_stop()

    def stopper(self):
        """Allow the controller to check final monitored-worker cleanup."""
        sc.wait(11, sc.SC_NS)


model = Model("monitor_join")
sc.sc_start()
assert join_records == [
    ("dynamic_start", 2, 0),
    ("clocked_start", 2, 0),
    ("late_add", 1, 2000),
    ("dynamic_done", 0, 3000),
    ("clocked_done", 0, 6000),
], join_records
assert sorted(cleanup_records) == sorted(
    [
        ("dynamic_start", 0),
        ("clocked_start", 0),
        ("dynamic_cleanup", 500),
        ("dynamic_start", 500),
        ("dynamic_cleanup", 1000),
        ("clocked_cleanup", 500),
        ("clocked_start", 500),
        ("clocked_cleanup", 2500),
    ]
), cleanup_records

print("MONITOR_PUBLIC_REFERENCE", int(sc.sc_process_monitor.spm_exit))
print("JOIN_PUBLIC_REFERENCE", *(f"{label}={count}@{time}" for label, count, time in join_records))
print("JOIN_CLEANUP_REFERENCE", *(f"{label}@{time}" for label, time in cleanup_records))
for label, report_id, severity, message_type, message, count in rejection_records:
    print("JOIN_REJECTION_REFERENCE", label, report_id, severity, message_type, message, count, sep="\t")
