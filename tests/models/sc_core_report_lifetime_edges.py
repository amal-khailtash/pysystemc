"""Check report timestamp references around copy assignment and swap."""

from amal.eda.systemc import sc_core as sc
import cppyy

reports = []


def capture_report(report, _actions):
    """Keep an owning report copy from the callback's borrowed report.

    Args:
        report: Native report valid for this callback only.
        _actions: Native report action mask, unused by this model.

    The retained report owns its strings and timestamp after the callback.
    Timestamp views obtained from it remain borrowed and must be reacquired after
    assignment. No view is retained across a mutating operation in this model.
    """
    reports.append(sc.sc_report(report))


def timestamp_snapshot(report):
    """Read a report timestamp address and value while its owner is live.

    Args:
        report: Live report owning the timestamp.

    Returns:
        A pair of native address and integer tick value. Only the integer address
        is retained; the borrowed ``sc_time`` proxy dies before mutation.
    """
    view = report.get_time()
    return cppyy.addressof(view), int(view.value())


def report_record(report):
    """Format owned report fields for exact native-reference comparison.

    Args:
        report: Live report whose fields are copied during formatting.

    Returns:
        Space-separated report metadata with the composed message encoded as hex.
    """
    composed = str(report.what())
    return (
        int(report.get_id()),
        int(report.get_severity()),
        str(report.get_msg_type()),
        str(report.get_msg()),
        str(report.get_file_name()),
        int(report.get_line_number()),
        int(report.get_time().value()),
        int(report.get_verbosity()),
        composed.encode("utf-8").hex(),
    )


sc.set_report_handler(capture_report)


def destination_worker():
    """Emit the initial report at 1 ns."""
    sc.wait(1, sc.SC_NS)
    sc.sc_report_handler.report(sc.SC_INFO, "assignment/destination", "old", "dest.cpp", 11)


def source_worker():
    """Emit the assignment source report at 2 ns."""
    sc.wait(2, sc.SC_NS)
    sc.sc_report_handler.report(sc.SC_WARNING, "assignment/source", "copied", "source.cpp", 73)


def alias_worker():
    """Emit the alternate report at 3 ns."""
    sc.wait(3, sc.SC_NS)
    sc.sc_report_handler.report(sc.SC_ERROR, "assignment/alias-target", "alias", "alias.cpp", 97)


processes = [
    sc.sc_spawn(destination_worker, "destination_worker"),
    sc.sc_spawn(source_worker, "source_worker"),
    sc.sc_spawn(alias_worker, "alias_worker"),
]
sc.sc_start(4, sc.SC_NS)
sc.set_report_handler()
assert all(process.terminated() for process in processes)
assert len(reports) == 3

destination, source, alias_target = reports
destination_address, destination_time = timestamp_snapshot(destination)
destination_address_again, _ = timestamp_snapshot(destination)
source_address, source_time = timestamp_snapshot(source)
alias_address, alias_time = timestamp_snapshot(alias_target)
assert destination_time == 1000 and source_time == 2000 and alias_time == 3000
assert destination_address == destination_address_again
assert len({destination_address, source_address, alias_address}) == 3
print("REPORT_TIME_REFERENCE", 1, 1, 1)

# Keep only the integer address. Assignment replaces and frees the destination's
# previous timestamp, so the old borrowed view is never retained or dereferenced.
returned = sc.sc_report.__assign__(destination, source)
assert returned is destination
assigned_address, assigned_time = timestamp_snapshot(destination)
assert assigned_address != destination_address and assigned_address != source_address
assert assigned_time == source_time
assert destination.get_process_name() == source.get_process_name()
print("REPORT_TIME_ASSIGN", 1, 1, assigned_time)
print("REPORT_TIME_CLASS_ASSIGN", 1, *report_record(destination))

# Native self-assignment also replaces its timestamp allocation. Snapshot first,
# then discard the view before assignment and reacquire only afterward.
self_old_address, self_old_time = timestamp_snapshot(destination)
returned_self = sc.sc_report.__assign__(destination, destination)
assert returned_self is destination
self_new_address, self_new_time = timestamp_snapshot(destination)
assert self_new_address != self_old_address
assert self_new_time == self_old_time == source_time
print("REPORT_TIME_SELF", 1, 1, self_new_time)

# The familiar instance spelling returns the exact Python destination proxy,
# copies every native report field and invalidates only its former time view.
instance_target = sc.sc_report(alias_target)
instance_old_address, _ = timestamp_snapshot(instance_target)
instance_result = instance_target.__assign__(source)
assert instance_result is instance_target
instance_new_address, instance_time = timestamp_snapshot(instance_target)
assert instance_new_address != instance_old_address and instance_new_address != source_address
assert instance_time == source_time
assert report_record(instance_target) == report_record(source)

# Instance self-assignment has the same destination identity and copy lifetime.
instance_self_address, instance_self_time = timestamp_snapshot(instance_target)
instance_self_result = instance_target.__assign__(instance_target)
assert instance_self_result is instance_target
instance_self_new_address, instance_self_new_time = timestamp_snapshot(instance_target)
assert instance_self_new_address != instance_self_address
assert instance_self_new_time == instance_self_time == source_time

# The third-report copy used before swapping goes through the explicit class
# adapter as well, so its returned owner identity is part of the supported API.
class_old_address, _ = timestamp_snapshot(destination)
class_assigned = sc.sc_report.__assign__(destination, alias_target)
assert class_assigned is destination
class_new_address, class_time = timestamp_snapshot(destination)
assert class_new_address != class_old_address and class_time == alias_time

# Both explicit class-adapter and native instance routes keep conversion errors
# before any mutation when the source cannot convert to sc_report.
before_bad_argument = report_record(destination)
for invoke in (
    lambda: sc.sc_report.__assign__(destination, object()),
    lambda: destination.__assign__(object()),
):
    try:
        invoke()
    except TypeError:
        pass
    else:
        raise AssertionError("invalid assignment source must retain TypeError")
assert report_record(destination) == before_bad_argument

# swap moves timestamp ownership between reports without replacing either value.
left_address, left_time = timestamp_snapshot(destination)
right_address, right_time = timestamp_snapshot(source)
destination.swap(source)
after_left_address, after_left_time = timestamp_snapshot(destination)
after_right_address, after_right_time = timestamp_snapshot(source)
assert after_left_address == right_address and after_right_address == left_address
assert (after_left_time, after_right_time) == (right_time, left_time)
print("REPORT_TIME_SWAP", 1, after_left_time, after_right_time, 1)

sc.sc_stop()
