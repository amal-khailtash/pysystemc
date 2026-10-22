"""Check native busy-list report errors, waiter timing and later recovery."""

from amal.eda.systemc import sc_core as sc


class Model(sc.sc_module):
    """Keep event-list storage and borrowed events alive through all waits."""

    def __init__(self, name):
        """Register two waiters and a driver with persistent native lists."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.a = sc.sc_event("a")
        self.b = sc.sc_event("b")
        self.c = sc.sc_event("c")
        self.or_list = sc.sc_event_or_list(self.a | self.b)
        self.and_list = sc.sc_event_and_list(self.a & self.b)
        self.errors = []
        self.times = []
        self.sc_thread("or_wait", self.or_wait)
        self.sc_thread("and_wait", self.and_wait)
        self.sc_thread("drive", self.drive)

    def or_wait(self):
        """Wait on persistent OR-list storage and record native wake time."""
        sc.wait(self.or_list)
        self.times.append(str(sc.sc_time_stamp()))

    def and_wait(self):
        """Wait until both borrowed events have notified."""
        sc.wait(self.and_list)
        self.times.append(str(sc.sc_time_stamp()))

    def drive(self):
        """Verify reports leave busy lists unchanged, then recover after wakeup."""
        sc.wait(1, sc.SC_NS)
        for cls, busy in ((sc.sc_event_or_list, self.or_list), (sc.sc_event_and_list, self.and_list)):
            idle = cls(self.c)
            failures = 0
            for operation in (
                lambda: busy.__assign__(idle),
                lambda: busy.swap(idle),
                lambda: idle.swap(busy),
                lambda: busy.swap(busy),
            ):
                try:
                    operation()
                except sc.sc_report as report:
                    assert "list modified while being waited on" in report.get_msg()
                    failures += 1
                else:
                    raise AssertionError("busy event-list mutation did not report")
                assert busy.size() == 2 and idle.size() == 1
            self.errors.append(failures)
        sc.wait(1, sc.SC_NS)
        self.a.notify()
        sc.wait(1, sc.SC_NS)
        self.b.notify()
        sc.wait(1, sc.SC_NS)
        for cls, former in ((sc.sc_event_or_list, self.or_list), (sc.sc_event_and_list, self.and_list)):
            idle = cls(self.c)
            former.__assign__(idle)
            assert former.size() == 1
            empty = cls()
            former.swap(empty)
            assert former.empty() and empty.size() == 1
        sc.sc_stop()


model = Model("busy")
sc.sc_start()
assert model.errors == [4, 4] and model.times == ["2 ns", "3 ns"]
print("EVENT_BUSY_REFERENCE", *model.errors, *model.times)
