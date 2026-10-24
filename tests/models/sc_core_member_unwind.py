"""Verify native member waits preserve Python reset/kill cleanup and event lists."""

from amal.eda.systemc import sc_core as sc
import cppyy

records = []
name = sc.sc_module_name("top")
module = sc.sc_module(name)
channel = sc.sc_prim_channel("channel")
del name
events = [sc.sc_event("first"), sc.sc_event("second")]
handles = []
halt_records = []
for owner in (module, channel):
    try:
        owner.timed_out()
    except cppyy.gbl.std.runtime_error as error:
        assert "timed_out requires a running SystemC process" in str(error)
    else:
        raise AssertionError("timeout query outside a process was accepted")


class HaltModule(sc.sc_module):
    """Verify member halt runs Python cleanup before native termination."""

    def __init__(self, name):
        """Register a clocked thread while the module-name scope is live."""
        super().__init__(name)
        self.clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
        self.clock_port = sc.sc_in[bool]("clock_port")
        self.clock_port(self.clock)
        self.sc_cthread("halt_worker", self.run, self.clock_port.pos())
        self.handle = sc.sc_get_last_created_process_handle()
        self.set_stack_size(2 * 1024 * 1024)

    def run(self):
        """Suspend at halt and record finally at its next native clock edge."""
        self.wait(1)
        try:
            halt_records.append(f"MEMBER_HALT start {sc.sc_time_stamp().value()}")
            self.halt()
            raise AssertionError("native halt returned normally")
        finally:
            halt_records.append(f"MEMBER_HALT cleanup {sc.sc_time_stamp().value()}")


halt_name = sc.sc_module_name("halt_top")
halt_module = HaltModule(halt_name)
del halt_name


def worker(owner, label, mode):
    """Block in an exact native member and record cleanup before reset/kill."""
    records.append(f"MEMBER_UNWIND {label} start {sc.sc_time_stamp().value()}")
    try:
        if mode == 0:
            owner.wait(sc.sc_time(20, sc.SC_NS))
        elif mode == 1:
            owner.wait(events[0] | events[1])
        else:
            owner.wait(events[0] & events[1])
        raise AssertionError("blocked member returned")
    finally:
        records.append(f"MEMBER_UNWIND {label} cleanup {sc.sc_time_stamp().value()}")


for owner, kind in ((module, "module"), (channel, "channel")):
    for mode in range(3):
        label = f"{kind}{mode}"
        handles.append(sc.sc_spawn(lambda owner=owner, label=label, mode=mode: worker(owner, label, mode), label))


def controller():
    """Reset then kill every blocked process while event targets remain alive."""
    sc.wait(2, sc.SC_NS)
    for handle in handles:
        handle.reset()
    sc.wait(1, sc.SC_NS)
    for handle in handles:
        handle.kill()


control = sc.sc_spawn(controller, "controller")
sc.sc_start(6, sc.SC_NS)
assert all(handle.terminated() for handle in handles) and control.terminated()
assert len(records) == 24, records
assert halt_module.handle.terminated()
assert halt_records == ["MEMBER_HALT start 2000", "MEMBER_HALT cleanup 4000"]
print("\n".join(records))
print("\n".join(halt_records))
sc.sc_stop()
