"""Verify pthread event lists, timeout waits, clocked threads and pause/resume."""

from amal.eda.systemc import sc_core as sc

records = []


class Model(sc.sc_module):
    """Own wait targets for the entire native thread lifetime."""

    def __init__(self, name):
        """Register threads and a clocked process with a retained edge finder."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.first = sc.sc_event("first")
        self.second = sc.sc_event("second")
        self.clock = sc.sc_clock("clock", sc.sc_time(2, sc.SC_NS))
        self.port = sc.sc_in[bool]("clock_port")
        self.port.bind(self.clock)
        self.edge = self.port.pos()
        self.sc_thread("events", self.events)
        self.sc_thread("drive", self.drive)
        self.sc_cthread("clocked", self.clocked, self.edge)

    def record(self, label):
        """Capture scheduler time for each observable resumption."""
        records.append((label, str(sc.sc_time_stamp())))

    def events(self):
        """Wait for either event, both events, then an event-or-timeout."""
        sc.wait(self.first | self.second)
        self.record("or")
        sc.wait(self.first & self.second)
        self.record("and")
        sc.wait(sc.sc_time(2, sc.SC_NS), self.first)
        self.record("timeout")
        sc.sc_pause()

    def drive(self):
        """Notify distinct events across simulation times."""
        sc.wait(1, sc.SC_NS)
        self.first.notify()
        sc.wait(1, sc.SC_NS)
        self.first.notify()
        sc.wait(1, sc.SC_NS)
        self.second.notify()

    def clocked(self):
        """Resume after one and then two native clock cycles."""
        self.record("clock_start")
        sc.wait()
        self.record("clock_one")
        sc.wait(2)
        self.record("clock_three")


model = Model("threads")
sc.sc_start(10, sc.SC_NS)
assert sc.sc_time_stamp() == sc.sc_time(5, sc.SC_NS)
assert sc.sc_get_status() == sc.SC_PAUSED
sc.sc_start(2, sc.SC_NS)
assert sc.sc_time_stamp() == sc.sc_time(7, sc.SC_NS)
assert sorted(records) == sorted(
    [
        ("or", "1 ns"),
        ("and", "3 ns"),
        ("timeout", "5 ns"),
        ("clock_start", "0 s"),
        ("clock_one", "2 ns"),
        ("clock_three", "6 ns"),
    ]
), records
sc.sc_stop()
for label, time in records:
    print(f"THREAD_RECORD {label}|{time}")
print("THREAD_SEMANTICS_OK")
