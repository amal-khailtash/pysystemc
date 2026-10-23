"""Compare public timeout and sampled-edge helpers to native SystemC."""

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt as dt
import cppyy


edge_records = []
timeout_records = {}
reset_starts = []
reset_cleanups = []
kill_starts = []
kill_cleanups = []


class Model(sc.sc_module):
    """Own sampled signals and the process callbacks for the comparison."""

    def __init__(self, name, clock):
        """Create targets and register clocked edge and timed waiter processes.

        Args:
            name: Native module name.
            clock: Clock retained by the caller through simulation.
        """
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock_port = sc.sc_in[bool]("clock_port")
        self.clock_port.bind(clock)
        self.edge_bool_pos = sc.sc_signal[bool]("edge_bool_pos", False)
        self.edge_bool_neg = sc.sc_signal[bool]("edge_bool_neg", False)
        self.edge_logic_pos = sc.sc_signal[dt.sc_logic]("edge_logic_pos", dt.sc_logic("0"))
        self.edge_logic_neg = sc.sc_signal[dt.sc_logic]("edge_logic_neg", dt.sc_logic("0"))
        self.reset_signal = sc.sc_signal[bool]("reset_signal", False)
        self.kill_signal = sc.sc_signal[bool]("kill_signal", False)
        self.early_event = sc.sc_event("early_event")
        self.late_event = sc.sc_event("late_event")
        self.sc_cthread("positive_bool", self.positive_bool, self.clock_port.pos())
        self.sc_cthread("negative_bool", self.negative_bool, self.clock_port.pos())
        self.sc_cthread("positive_logic", self.positive_logic, self.clock_port.pos())
        self.sc_cthread("negative_logic", self.negative_logic, self.clock_port.pos())
        self.sc_cthread("reset_edge_wait", self.reset_edge_wait, self.clock_port.pos())
        self.reset_handle = sc.sc_get_last_created_process_handle()
        self.sc_cthread("kill_edge_wait", self.kill_edge_wait, self.clock_port.pos())
        self.kill_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("early_timeout", self.early_timeout)
        self.sc_thread("late_timeout", self.late_timeout)
        self.sc_thread("drive", self.drive)
        self.sc_thread("control_unwinds", self.control_unwinds)

    def record_edge(self, label):
        """Record helper completion label and exact native time."""
        edge_records.append((label, int(sc.sc_time_stamp().value())))

    def positive_bool(self):
        """Wait for the next rising transition on a bool signal."""
        sc.at_posedge(self.edge_bool_pos)
        self.record_edge("pos_bool")

    def negative_bool(self):
        """Wait for the next falling transition on a bool signal."""
        sc.at_negedge(self.edge_bool_neg)
        self.record_edge("neg_bool")

    def positive_logic(self):
        """Wait for the next rising transition on an sc_logic signal."""
        sc.at_posedge(self.edge_logic_pos)
        self.record_edge("pos_logic")

    def negative_logic(self):
        """Wait for the next falling transition on an sc_logic signal."""
        sc.at_negedge(self.edge_logic_neg)
        self.record_edge("neg_logic")

    def early_timeout(self):
        """Check timed_out after an event wins over a five-nanosecond timeout."""
        sc.wait(sc.sc_time(5, sc.SC_NS), self.early_event)
        context = sc.sc_get_curr_simcontext()
        zero_argument = bool(sc.timed_out())
        explicit_context = bool(sc.timed_out(context))
        timeout_records["early"] = (zero_argument, explicit_context, int(sc.sc_time_stamp().value()))

    def late_timeout(self):
        """Check timed_out after a three-nanosecond timeout expires."""
        sc.wait(sc.sc_time(3, sc.SC_NS), self.late_event)
        context = sc.sc_get_curr_simcontext()
        zero_argument = bool(sc.timed_out())
        explicit_context = bool(sc.timed_out(context))
        timeout_records["late"] = (zero_argument, explicit_context, int(sc.sc_time_stamp().value()))

    def reset_edge_wait(self):
        """Leave cleanup evidence when reset unwinds the native edge wait."""
        reset_starts.append(int(sc.sc_time_stamp().value()))
        try:
            sc.at_posedge(self.reset_signal)
        finally:
            reset_cleanups.append(int(sc.sc_time_stamp().value()))

    def kill_edge_wait(self):
        """Leave cleanup evidence when process kill unwinds the native edge wait."""
        kill_starts.append(int(sc.sc_time_stamp().value()))
        try:
            sc.at_posedge(self.kill_signal)
        finally:
            kill_cleanups.append(int(sc.sc_time_stamp().value()))

    def drive(self):
        """Drive the timeout event and both polarities for bool and logic signals."""
        sc.wait(2, sc.SC_NS)
        self.early_event.notify()
        self.edge_bool_pos.write(True)
        self.edge_bool_neg.write(True)
        self.edge_logic_pos.write(dt.sc_logic("1"))
        self.edge_logic_neg.write(dt.sc_logic("1"))
        sc.wait(2, sc.SC_NS)
        self.edge_bool_pos.write(False)
        self.edge_bool_neg.write(False)
        self.edge_logic_pos.write(dt.sc_logic("0"))
        self.edge_logic_neg.write(dt.sc_logic("0"))

    def control_unwinds(self):
        """Reset then kill one blocked edge wait and kill a second blocked wait."""
        sc.wait(sc.sc_time(500, sc.SC_PS))
        self.reset_handle.reset()
        sc.wait(sc.sc_time(500, sc.SC_PS))
        self.reset_handle.kill()
        self.kill_handle.kill()
        sc.wait(5, sc.SC_NS)
        sc.sc_stop()


clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
model = Model("edge_waits", clock)
sc.sc_start()

assert edge_records == [
    ("pos_logic", 2000),
    ("pos_bool", 2000),
    ("neg_bool", 4000),
    ("neg_logic", 4000),
], edge_records
assert timeout_records == {"early": (False, False, 2000), "late": (True, True, 3000)}, timeout_records
assert reset_starts == [0, 500] and reset_cleanups == [500, 1000], (reset_starts, reset_cleanups)
assert kill_starts == [0] and kill_cleanups == [1000], (kill_starts, kill_cleanups)

try:
    sc.timed_out()
except cppyy.gbl.std.runtime_error as error:
    outside_context_error = str(error)
else:
    raise AssertionError("timed_out outside a running process did not fail")
assert "timed_out requires a running SystemC process" in outside_context_error, outside_context_error

print("EDGE_WAIT_REFERENCE", *(f"{label}@{time}" for label, time in edge_records))
print(
    "TIMED_OUT_REFERENCE",
    *(f"{label}={int(zero)}/{int(explicit)}@{time}" for label, (zero, explicit, time) in timeout_records.items()),
)
print(
    "EDGE_CLEANUP_REFERENCE", f"reset={','.join(map(str, reset_cleanups))}", f"kill={','.join(map(str, kill_cleanups))}"
)
print("TIMED_OUT_CONTEXT_GUARD timed_out requires a running SystemC process")

# Import cppyy only after SystemC setup, then confirm both views share the stopped kernel.
assert cppyy.gbl.sc_core.sc_get_status() == sc.SC_STOPPED
