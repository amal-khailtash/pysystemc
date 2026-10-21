"""Compare native report copies, getters, swap and composed messages."""

from amal.eda.systemc import sc_core as sc

owned = []
records = []


def handler(report, actions):
    """Copy borrowed callback report before native reporting destroys it."""
    copy = sc.sc_report(report)
    owned.append(copy)
    assert copy.valid() == (copy.get_process_name() == "report_worker")
    assert copy.get_id() == -1
    assert copy.get_msg_type() == "public/report"
    assert copy.get_file_name() == "model.cpp" and copy.get_line_number() == 42
    assert copy.get_time().value() in (0, 2000)
    composed = str(sc.sc_report_compose_message(copy))
    assert composed == copy.what()
    records.append("REPORT_VALUE_REFERENCE " + composed.encode().hex())


sc.set_report_handler(handler)
sc.sc_report_handler.report(sc.SC_INFO, "public/report", "first", "model.cpp", 42)
sc.sc_report_handler.report(sc.SC_WARNING, "public/report", "second", "model.cpp", 42)
sc.set_report_handler()
assert len(owned) == 2
first, second = owned
assert first.get_msg() == "first" and second.get_msg() == "second"
assert first.get_severity() == sc.SC_INFO and second.get_severity() == sc.SC_WARNING
assert first.get_verbosity() == second.get_verbosity() == sc.SC_MEDIUM
first.swap(second)
assert first.get_msg() == "second" and second.get_msg() == "first"
assert str(sc.sc_report_compose_message(first)) == first.what()


def worker():
    """Report at2ns while process identity and timestamp are meaningful."""
    sc.wait(2, sc.SC_NS)
    sc.sc_report_handler.report(sc.SC_ERROR, "public/report", "thread", "model.cpp", 42)


sc.set_report_handler(handler)
process = sc.sc_spawn(worker, "report_worker")
sc.sc_start(3, sc.SC_NS)
sc.set_report_handler()
assert process.terminated() and len(owned) == 3
assert owned[2].valid() and owned[2].get_process_name() == "report_worker"
assert owned[2].get_time().value() == 2000
sc.sc_stop()
for record in records:
    print(record)
print("REPORT_COPY_REFERENCE second first 0")
