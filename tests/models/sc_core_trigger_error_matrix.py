"""Check exact next_trigger reports across every free overload and context form."""

from amal.eda.systemc import sc_core as sc

first, second = sc.sc_event("first"), sc.sc_event("second")
or_list, and_list = sc.sc_event_or_list(first | second), sc.sc_event_and_list(first & second)
duration = sc.sc_time(1, sc.SC_NS)
records = []


def arguments():
    """Create fresh single-use expressions alongside reusable borrowed lists."""
    return {
        "none": (),
        "event": (first,),
        "or_list": (or_list,),
        "and_list": (and_list,),
        "time": (duration,),
        "units": (1, sc.SC_NS),
        "time_event": (duration, first),
        "units_event": (1, sc.SC_NS, first),
        "time_or": (duration, or_list),
        "units_or": (1, sc.SC_NS, or_list),
        "time_and": (duration, and_list),
        "units_and": (1, sc.SC_NS, and_list),
        "or_expr": (first | second,),
        "and_expr": (first & second,),
        "time_or_expr": (duration, first | second),
        "units_or_expr": (1, sc.SC_NS, first | second),
        "time_and_expr": (duration, first & second),
        "units_and_expr": (1, sc.SC_NS, first & second),
    }


def worker():
    """Recover from exactly one native E520 report for each selected overload."""
    for explicit in (False, True):
        for name, values in arguments().items():
            label = name + ("_explicit" if explicit else "_default")
            before = sc.sc_report_handler.get_count(sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_)
            if explicit:
                values = (*values, sc.sc_get_curr_simcontext())
            try:
                sc.next_trigger(*values)
            except sc.sc_report as error:
                assert error.get_id() == 520
                assert error.get_severity() == sc.SC_ERROR
                assert error.get_msg_type() == sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_
                count = sc.sc_report_handler.get_count(sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_) - before
                assert count == 1, (label, count)
                assert sc.sc_time_stamp() == sc.SC_ZERO_TIME and sc.sc_delta_count() == 0
                records.append(
                    f"TRIGGER_ERROR {label}|{error.get_id()}|{int(error.get_severity())}|{error.get_msg_type()}|"
                    f"{' '.join(error.get_msg().split())}|{sc.sc_time_stamp().value()}|{sc.sc_delta_count()}|{count}"
                )
            else:
                raise AssertionError("invalid thread next_trigger returned: " + label)


for value in (object(), "bad"):
    try:
        sc.next_trigger(value)
    except TypeError:
        pass
    else:
        raise AssertionError("invalid host argument accepted")
assert sc.sc_get_status() == sc.SC_ELABORATION
assert sc.sc_time_stamp() == sc.SC_ZERO_TIME and sc.sc_delta_count() == 0
previous = sc.sc_report_handler.set_actions(sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, sc.SC_ERROR, sc.SC_THROW)
handle = sc.sc_spawn(worker, "trigger_errors")
sc.sc_start(1, sc.SC_NS)
assert handle.terminated() and len(records) == 36
assert sc.sc_report_handler.get_count(sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_) == 36
sc.sc_report_handler.set_actions(sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, sc.SC_ERROR, previous)
sc.sc_stop()
for record in records:
    print(record)
print("TRIGGER_ERROR_OK reports=36 invalid_arguments=2")
