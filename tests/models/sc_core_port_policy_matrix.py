"""Check all port policies at empty, partial, full and excess binding counts."""

from amal.eda.systemc import sc_core as sc

reports = []


def report(record, actions):
    """Copy native elaboration diagnostics without retaining borrowed report storage."""
    reports.append((record.get_id(), int(record.get_severity()), record.get_msg_type(), record.get_msg()))


cases = []
for policy_label, policy in (
    ("one", sc.SC_ONE_OR_MORE_BOUND),
    ("zero", sc.SC_ZERO_OR_MORE_BOUND),
    ("all", sc.SC_ALL_BOUND),
):
    for maximum, counts in ((2, (0, 1, 2, 3)), (0, (0, 1, 3))):
        for count in counts:
            name = sc.sc_module_name(f"port_policy_{len(cases)}")
            top = sc.sc_module(name)
            signals = [sc.sc_signal[int](f"source_{index}", value) for index, value in enumerate((17, 29, 43))]
            port = sc.sc_port[sc.sc_signal_in_if[int], maximum, policy]("required")
            for signal in signals[:count]:
                port.bind(signal)
            assert port.bind_count() == count
            cases.append((policy_label, maximum, count, top, signals, port))
            del name

sc.set_report_handler(report)
sc.sc_start(sc.SC_ZERO_TIME)
sc.set_report_handler()
assert sc.sc_get_status() == sc.SC_PAUSED
assert len(reports) == 8, reports
for index, (label, maximum, count, top, signals, port) in enumerate(cases):
    assert port.size() == count
    values = [int(port.get_interface(offset).read()) for offset in range(count)]
    assert values == [17, 29, 43][:count]
    selected = [record for record in reports if f"port 'port_policy_{index}.required'" in record[3]]
    assert all(record[:3] == (109, int(sc.SC_ERROR), sc.SC_ID_COMPLETE_BINDING_) for record in selected)
    details = ";".join(record[3] for record in selected)
    print(f"PORT_POLICY|{label}|{maximum}|{count}|{port.size()}|{','.join(map(str, values))}|{len(selected)}|{details}")
print("PORT_POLICY_DONE cases=21 reports=8 time=0")
