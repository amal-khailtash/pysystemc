"""Check sampled-edge member TypeError recovery and native sample values."""

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import sc_dt as dt


class EdgeRecoveryModel(sc.sc_module):
    """Own unsupported and supported interfaces for sampled member waits."""

    def __init__(self, name):
        """Create clocked wait and driver processes with bool and logic signals."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
        self.clock_port = sc.sc_in[bool]("clock_port")
        self.clock_port.bind(self.clock)
        self.unsupported = sc.sc_signal[int]("unsupported", 0)
        self.bool_signal = sc.sc_signal[bool]("bool_signal", False)
        self.logic_signal = sc.sc_signal[dt.sc_logic]("logic_signal", dt.sc_logic("0"))
        self.kill_signal = sc.sc_signal[bool]("kill_signal", False)
        self.invalid_errors = []
        self.samples = []
        self.kill_cleanups = []
        self.sc_cthread("recover_edges", self.recover_edges, self.clock_port.pos())
        self.sc_cthread("killed_edge_wait", self.killed_edge_wait, self.clock_port.pos())
        self.kill_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("drive_edges", self.drive_edges)
        self.sc_thread("kill_edge_wait", self.kill_edge_wait)

    def recover_edges(self):
        """Catch invalid bool/logic interface types and continue with valid waits."""
        for method in (self.at_posedge, self.at_negedge):
            try:
                method(self.unsupported)
            except TypeError as error:
                self.invalid_errors.append(str(error))
            else:
                raise AssertionError("unsupported sampled-edge input did not raise TypeError")

        self.at_posedge(self.bool_signal)
        self.samples.append(("pos", bool(self.bool_signal.read()), int(sc.sc_time_stamp().value())))
        self.at_negedge(self.logic_signal)
        self.samples.append(("neg", str(self.logic_signal.read()), int(sc.sc_time_stamp().value())))

    def killed_edge_wait(self):
        """Remain in a bool sampled-edge wait until native kill runs cleanup."""
        try:
            self.at_posedge(self.kill_signal)
            raise AssertionError("killed sampled-edge wait returned normally")
        finally:
            self.kill_cleanups.append(int(sc.sc_time_stamp().value()))

    def drive_edges(self):
        """Drive recovery edges, leaving the kill target signal unchanged."""
        sc.wait(2, sc.SC_NS)
        self.bool_signal.write(True)
        sc.wait(2, sc.SC_NS)
        self.logic_signal.write(dt.sc_logic("1"))
        sc.wait(2, sc.SC_NS)
        self.logic_signal.write(dt.sc_logic("0"))
        sc.wait(1, sc.SC_NS)
        sc.sc_stop()

    def kill_edge_wait(self):
        """Kill a blocked bool sampled-edge wait at three nanoseconds."""
        sc.wait(3, sc.SC_NS)
        self.kill_handle.kill()


model = EdgeRecoveryModel("sampled_edge_recovery")
sc.sc_start()

assert len(model.invalid_errors) == 2, model.invalid_errors
assert all(
    "sampled edge waits require a native bool/sc_logic input interface" in error for error in model.invalid_errors
)
assert model.samples == [("pos", True, 2000), ("neg", "0", 6000)], model.samples
assert model.kill_cleanups == [3000], model.kill_cleanups
assert model.kill_handle.terminated()
print(
    "SAMPLED_EDGE_RECOVERY",
    f"pos={model.samples[0][1]}@{model.samples[0][2]}",
    f"neg={model.samples[1][1]}@{model.samples[1][2]}",
    f"invalid={len(model.invalid_errors)}",
    f"kill={','.join(map(str, model.kill_cleanups))}",
)
