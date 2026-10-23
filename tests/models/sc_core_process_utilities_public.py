"""Exercise halt, stack configuration, status and process-handle operators."""

from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_process_utilities_probe.h"))
records = []
for status_name, status in sc.ScStatus.__members__.items():
    assert status.value == int(getattr(cppyy.gbl.sc_core, status_name))
    assert int(cppyy.gbl.sc_core.sc_status(status.value)) == status.value


class Model(sc.sc_module):
    """Retain halted clocked threads and their controller through simulation."""

    def __init__(self):
        """Configure live thread stacks and register both halt context forms."""
        name = sc.sc_module_name("utilities")
        super().__init__(name)
        self.clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
        self.clock_input = sc.sc_in[bool]("clock_input")
        self.clock_input.bind(self.clock)
        self.sc_cthread("default", self.default, self.clock_input.pos())
        self.default_handle = sc.sc_get_last_created_process_handle()
        self.sc_cthread("explicit", self.explicit, self.clock_input.pos())
        self.explicit_handle = sc.sc_get_last_created_process_handle()
        self.sc_cthread("reset", self.reset, self.clock_input.pos())
        self.reset_handle = sc.sc_get_last_created_process_handle()
        self.sc_cthread("kill", self.kill, self.clock_input.pos())
        self.kill_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("drive", self.drive)
        self.drive_handle = sc.sc_get_last_created_process_handle()
        sc.sc_set_stack_size(self.default_handle, 2 * 1024 * 1024)
        sc.sc_set_stack_size(cppyy.gbl.process_utilities_probe.thread(self.explicit_handle), 3 * 1024 * 1024)
        assert cppyy.gbl.process_utilities_probe.stack_probe.size(self.default_handle) == 2 * 1024 * 1024
        assert cppyy.gbl.process_utilities_probe.stack_probe.size(self.explicit_handle) == 3 * 1024 * 1024
        for invalid_size, error in ((0, ValueError), (-1, ValueError), (True, TypeError), (1.5, TypeError)):
            try:
                sc.sc_set_stack_size(self.default_handle, invalid_size)
            except error:
                pass
            else:
                raise AssertionError("invalid stack size was accepted")
        for invalid_handle in (sc.sc_process_handle(), None, cppyy.nullptr):
            try:
                sc.sc_set_stack_size(invalid_handle, 1024)
            except ValueError:
                pass
            else:
                raise AssertionError("invalid thread handle was accepted")
        del name

    def halted(self, label, explicit=False):
        """Record native halt entry/cleanup and forbid returning past the halt."""
        records.append((label, "start", sc.sc_time_stamp().value()))
        try:
            sc.halt(sc.sc_get_curr_simcontext()) if explicit else sc.halt()
        finally:
            records.append((label, "cleanup", sc.sc_time_stamp().value()))
        raise AssertionError("halt returned normally")

    def default(self):
        """Halt with the native default context."""
        self.halted("default")

    def explicit(self):
        """Halt with the explicit current context."""
        self.halted("explicit", True)

    def reset(self):
        """Restart a suspended halt and observe both cleanup paths."""
        self.halted("reset")

    def kill(self):
        """Observe kill cleanup while the native halt remains suspended."""
        self.halted("kill")

    def drive(self):
        """Control halted threads independently, proving suspension releases GIL."""
        assert sc.sc_status() == sc.ScStatus.SC_RUNNING
        sc.wait(0.5, sc.SC_NS)
        self.reset_handle.reset()
        sc.wait(0.5, sc.SC_NS)
        self.kill_handle.kill()
        sc.wait(4, sc.SC_NS)
        assert all(
            handle.terminated()
            for handle in (self.default_handle, self.explicit_handle, self.reset_handle, self.kill_handle)
        )
        sc.sc_stop()


def invalid_halt():
    """Compare the native outside-clocked-process halt report without entering simulation."""
    sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_THROW)
    try:
        sc.halt()
    except sc.sc_report as report:
        print("HALT_ERROR", report.get_id(), int(report.get_severity()), report.get_msg_type(), sep="\t")
    else:
        raise AssertionError("halt outside a clocked thread was accepted")


if len(sys.argv) > 1:
    invalid_halt()
else:
    assert sc.sc_status() == sc.ScStatus.SC_ELABORATION
    model = Model()
    first, second = model.default_handle, model.explicit_handle
    copied = sc.sc_process_handle(first)
    empty, another_empty = sc.sc_process_handle(), sc.sc_process_handle()
    assert first == copied and not first != copied
    assert first != second and not first == second
    assert not empty == another_empty and empty != another_empty
    assert not first < first and not empty < another_empty
    assert (first < second) == cppyy.gbl.process_utilities_probe.less(first, second)
    assert (second < first) == cppyy.gbl.process_utilities_probe.less(second, first)
    assert (first < second) != (second < first)
    print("PROCESS_COMPARE 1 1 1 1 1")
    print("STACK_CONFIG 2097152 3145728")
    sc.sc_start()
    assert sc.sc_status() == sc.ScStatus.SC_STOPPED
    assert sc.sc_time_stamp() == sc.sc_time(5, sc.SC_NS)
    expected = [
        ("default", "start", 0),
        ("default", "cleanup", 2000),
        ("explicit", "start", 0),
        ("explicit", "cleanup", 2000),
        ("reset", "start", 0),
        ("reset", "cleanup", 500),
        ("reset", "start", 500),
        ("reset", "cleanup", 2000),
        ("kill", "start", 0),
        ("kill", "cleanup", 1000),
    ]
    assert sorted(records) == sorted(expected), records
    for label, kind, time in sorted(records):
        print("HALT_RECORD", label, kind, time)
    assert first == copied and first != second
    print("PROCESS_FINAL 5000 64")
