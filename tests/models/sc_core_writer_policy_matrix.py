"""Emit writer-policy reports and signal transitions from a Python model."""

from amal.eda.systemc import sc_core as sc

reports = []


def capture(report, actions):
    """Copy report identity and text while suppressing expected E115 output."""
    reports.append(
        (
            int(report.get_id()),
            int(report.get_severity()),
            report.get_msg_type(),
            str(sc.sc_time_stamp()),
            int(sc.sc_delta_count()),
            report.get_msg().replace("\n", "~"),
        )
    )


def emit_reports():
    """Emit captured reports in native callback order with portable separators."""
    phases = (("one", "equal_same_delta"), ("many", "equal_same_delta"), ("one", "later_owner"))
    for (policy, phase), item in zip(phases, reports, strict=True):
        report_id, severity, message_type, time, delta, message = item
        print(
            "WPM\tREPORT\t"
            + "\t".join(map(str, (policy, phase, report_id, severity, message_type, time, delta, message)))
        )


class WriterPolicyMatrix(sc.sc_module):
    """Own three policy signals and sample every value change."""

    def __init__(self):
        """Register two writer threads and one observer method per policy."""
        module_name = sc.sc_module_name("top")
        super().__init__(module_name)
        self.channels = (
            ("one", sc.sc_signal[int, sc.SC_ONE_WRITER]("signal_one")),
            ("many", sc.sc_signal[int, sc.SC_MANY_WRITERS]("signal_many")),
            ("unchecked", sc.sc_signal[int, sc.SC_UNCHECKED_WRITERS]("signal_unchecked")),
        )
        self.samples = []
        self.sc_thread("first", self.first)
        self.sc_thread("second", self.second)
        for policy, signal in self.channels:
            self.sc_method(f"observe_{policy}", lambda policy=policy, signal=signal: self.observe(policy, signal))
            self.sensitive(signal)
            self.dont_initialize()

    def observe(self, policy, signal):
        """Record value changes with native simulation time and delta count."""
        self.samples.append((policy, str(signal.read()), str(sc.sc_time_stamp()), int(sc.sc_delta_count())))

    def first(self):
        """Write the common pending value, then a later-time final value."""
        for _, signal in self.channels:
            signal.write(1)
        sc.wait(2, sc.SC_NS)
        for _, signal in self.channels:
            signal.write(3)

    def second(self):
        """Repeat the pending value and change it in the next delta cycle."""
        for _, signal in self.channels:
            signal.write(1)
        sc.wait(sc.SC_ZERO_TIME)
        for _, signal in self.channels:
            signal.write(2)
        sc.wait(1, sc.SC_NS)


sc.set_report_handler(capture)
try:
    model = WriterPolicyMatrix()
    sc.sc_start(3, sc.SC_NS)
finally:
    sc.set_report_handler()

expected_reports = (
    (115, int(sc.SC_ERROR), "sc_signal<T> cannot have more than one driver", "0 s", 0),
    (115, int(sc.SC_ERROR), "sc_signal<T> cannot have more than one driver", "0 s", 0),
    (115, int(sc.SC_ERROR), "sc_signal<T> cannot have more than one driver", "2 ns", 4),
)
assert tuple(item[:5] for item in reports) == expected_reports, reports
assert "signal `top.signal_one'" in reports[0][5]
assert "first driver `top.first'" in reports[0][5]
assert "second driver `top.second'" in reports[0][5]
assert "signal `top.signal_many'" in reports[1][5]
assert "conflicting write in delta cycle 0" in reports[1][5]
assert "signal `top.signal_one'" in reports[2][5]
assert "first driver `top.second'" in reports[2][5]
assert "second driver `top.first'" in reports[2][5]
assert all("signal `top.signal_unchecked'" not in item[5] for item in reports)
emit_reports()

expected_samples = tuple(
    (policy, value, time, delta)
    for time, value, delta in (("0 s", "1", 1), ("0 s", "2", 2), ("2 ns", "3", 5))
    for policy in ("one", "many", "unchecked")
)
assert tuple(model.samples) == expected_samples, model.samples
for policy, value, time, delta in model.samples:
    print("WPM\tSAMPLE\t" + "\t".join(map(str, (policy, value, time, delta))))

for policy, signal in model.channels:
    assert signal.read() == 3
    print("WPM\tFINAL\t" + "\t".join(map(str, (policy, signal.read(), sc.sc_time_stamp(), sc.sc_delta_count()))))

assert sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
print("SC_CORE_WRITER_POLICY_MATRIX_OK")
