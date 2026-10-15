"""Validate writer contention reports for all native signal policies."""

from amal.eda.systemc import sc_core as sc

reports = []


def report(record, actions):
    """Collect native contention reports without aborting the bounded simulation."""
    reports.append((record.get_msg_type(), record.get_msg()))


sc.set_report_handler(report)
signals = [
    sc.sc_signal[int, policy](f"signal_{int(policy)}")
    for policy in (sc.SC_ONE_WRITER, sc.SC_MANY_WRITERS, sc.SC_UNCHECKED_WRITERS)
]


def first():
    """Drive initially and reclaim each signal in a later delta."""
    for signal in signals:
        signal.write(1)
    sc.wait(1, sc.SC_NS)
    for signal in signals:
        signal.write(4)


def second():
    """Conflict with the first writer in the initial delta."""
    for signal in signals:
        signal.write(2)


sc.sc_spawn(first, "first")
sc.sc_spawn(second, "second")
sc.sc_start(2, sc.SC_NS)
assert len(reports) == 3, reports
assert sum("signal_0" in message for _, message in reports) == 2, reports
assert sum("signal_1" in message for _, message in reports) == 1, reports
assert all("signal_3" not in message for _, message in reports), reports
assert [signal.read() for signal in signals] == [4, 4, 4]
sc.set_report_handler()
sc.sc_stop()
print("WRITER_POLICIES_OK")
