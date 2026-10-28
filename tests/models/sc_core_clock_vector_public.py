"""Compare public sc_clock and sc_pvector behavior with the native reference."""

from amal.eda.systemc import sc_core as sc
import cppyy
import gc
import pathlib

cppyy.include(str(pathlib.Path(__file__).resolve().parents[1] / "native" / "sc_core_clock_vector_probe.h"))
probe = cppyy.gbl.sc_core_clock_vector_probe


def rejected(action, expected_id):
    """Require an invalid native clock operation to raise its expected report.

    Args:
        action: Callable that performs one operation expected to raise sc_report.
        expected_id: SystemC report identifier expected for the operation.

    Returns:
        The matching native SystemC report identifier.

    Raises:
        AssertionError: If the operation does not report an error.
    """
    try:
        action()
    except sc.sc_report as error:
        assert error.get_id() == expected_id
        return error.get_id()
    except Exception as error:
        # cppyy may wrap a constructor sc_report in its overload diagnostics.
        assert f"(E{expected_id})" in str(error)
        return expected_id
    raise AssertionError("native clock error was not reported")


# All overloads are distinct native constructors from sc_clock.h.
default_clock = sc.sc_clock()
named_clock = sc.sc_clock("clock_named")
time_clock = sc.sc_clock("clock_time", sc.sc_time(8, sc.SC_NS), 0.25, sc.sc_time(2, sc.SC_NS), False)
short_clock = sc.sc_clock("clock_short", 10, sc.SC_NS, 0.3)
short_default_duty_clock = sc.sc_clock("clock_short_default", 6, sc.SC_NS)
long_clock = sc.sc_clock("clock_long", 12, sc.SC_NS, 0.25, 3, sc.SC_NS, False)
legacy_clock = sc.sc_clock("clock_legacy", 14, 0.5, 2, False)
fallback_clock = sc.sc_clock("clock_fallback", sc.sc_time(4, sc.SC_NS), 0.0)
keyword_unit_clock = sc.sc_clock("clock_keyword_unit", 10, period_tu_=sc.SC_NS)
keyword_duty_clock = sc.sc_clock("clock_keyword_duty", 10, period_tu_=sc.SC_NS, duty_cycle_=0.25)
mixed_keyword_clock = sc.sc_clock("clock_mixed_keyword", 10, sc.SC_NS, duty_cycle_=0.25)

for clock, duty in ((keyword_unit_clock, 0.5), (keyword_duty_clock, 0.25), (mixed_keyword_clock, 0.25)):
    assert clock.period() == sc.sc_time(10, sc.SC_NS)
    assert clock.duty_cycle() == duty
    assert clock.start_time() == sc.SC_ZERO_TIME and clock.posedge_first()

assert default_clock.period() == sc.sc_time(1, sc.SC_NS)
assert named_clock.period() == sc.sc_time(1, sc.SC_NS)
assert time_clock.kind() == "sc_clock"
assert time_clock.period() == sc.sc_time(8, sc.SC_NS)
assert time_clock.duty_cycle() == 0.25
assert time_clock.start_time() == sc.sc_time(2, sc.SC_NS)
assert not time_clock.posedge_first()
assert short_clock.period() == sc.sc_time(10, sc.SC_NS)
assert abs(short_clock.duty_cycle() - 0.3) < 1e-12 and short_clock.posedge_first()
assert short_clock.start_time() == sc.SC_ZERO_TIME
assert short_default_duty_clock.period() == sc.sc_time(6, sc.SC_NS)
assert short_default_duty_clock.duty_cycle() == 0.5
assert short_default_duty_clock.start_time() == sc.SC_ZERO_TIME and short_default_duty_clock.posedge_first()
assert long_clock.period() == sc.sc_time(12, sc.SC_NS)
assert long_clock.start_time() == sc.sc_time(3, sc.SC_NS) and not long_clock.posedge_first()
assert legacy_clock.period() == sc.sc_time(14, sc.SC_NS)
assert legacy_clock.start_time() == sc.sc_time(2, sc.SC_NS) and not legacy_clock.posedge_first()
assert fallback_clock.duty_cycle() == 0.5
protected_clock = bool(probe.protected_clock_helper())
assert protected_clock
assert sc.sc_clock.time_stamp() == sc.sc_time_stamp()

report_ids = [rejected(lambda: sc.sc_clock("clock_zero", sc.SC_ZERO_TIME), 101)]
report_ids.append(
    rejected(
        lambda: sc.sc_clock("clock_high_zero", sc.sc_time(1, sc.SC_PS), 0.000001, sc.SC_ZERO_TIME, True),
        102,
    )
)
report_ids.append(
    rejected(
        lambda: sc.sc_clock("clock_low_zero", sc.sc_time(1, sc.SC_PS), 0.999999, sc.SC_ZERO_TIME, True),
        103,
    )
)
report_ids.append(rejected(lambda: time_clock.write(True), 125))
report_ids.append(probe.output_port_registration_report_id(time_clock))
assert report_ids == [101, 102, 103, 125, 128]

# Run all native edge processes far enough to observe both start offsets and
# duty-cycle transitions. Each sc_start advances this fresh process's kernel.
assert not default_clock.read() and not named_clock.read()
assert time_clock.read() and long_clock.read() and legacy_clock.read()
sc.sc_start(2.25, sc.SC_NS)
assert default_clock.read() and named_clock.read()
assert not time_clock.read() and short_clock.read() and long_clock.read() and not legacy_clock.read()
sc.sc_start(1, sc.SC_NS)
assert not short_clock.read() and not long_clock.read()
assert sc.sc_time_stamp() == sc.sc_time(3.25, sc.SC_NS)
assert sc.sc_clock.time_stamp() == sc.sc_time_stamp()

int_vector_type = sc.sc_pvector["int*"]
items = int_vector_type()
for index in range(3):
    items.push_back(probe.integer_pointer(index))
assert items.size() == 3
assert probe.integer_value(items.fetch(1)) == 1
assert probe.integer_value(items[0]) == 3
items[4] = probe.integer_pointer(3)
assert items.size() == 5 and probe.integer_vector_slot_is_null(items, 3)
assert not bool(items[3])
assert probe.integer_vector_slot_is_null(items, 5) and items.size() == 6
items.decr_count()
for bad_index in (-1, 5):
    try:
        items.fetch(bad_index)
    except IndexError:
        pass
    else:
        raise AssertionError(f"fetch accepted out-of-range index {bad_index}")
try:
    items[-1]
except IndexError:
    pass
else:
    raise AssertionError("operator[] accepted a negative index")
try:
    items[-1] = probe.integer_pointer(0)
except IndexError:
    pass
else:
    raise AssertionError("operator[] assignment accepted a negative index")
items.put(probe.integer_pointer(4), 3)
assert probe.fetched_integer_value(items, 3) == 5
assert probe.converted_size["int*"](items) == 5
assert probe.iterator_storage_agrees["int*"](items)
assert probe.const_storage_agrees["int*"](items)

copied = int_vector_type(items)
assigned = int_vector_type()
probe.assign["int*"](assigned, copied)
assert assigned.size() == copied.size()
assert probe.fetched_integer_value(assigned, 4) == 1

comparison_pairs = []


def compare_integer_slots(left, right):
    """Compare vector entries by their borrowed integer pointees.

    Args:
        left: Native pointer to a vector slot supplied by qsort.
        right: Native pointer to a vector slot supplied by qsort.

    Returns:
        Negative, zero, or positive according to pointee order.
    """
    lhs = probe.integer_slot_value(left)
    rhs = probe.integer_slot_value(right)
    comparison_pairs.append((lhs, rhs))
    return (lhs > rhs) - (lhs < rhs)


items.sort(compare_integer_slots)
assert comparison_pairs
sorted_values = tuple(probe.integer_vector_value(items, index) for index in range(items.size()))
assert sorted_values == (1, 1, 3, 4, 5)
assert probe.fetched_integer_value(copied, 0) == 3  # copy owns vector slots, not pointees
items.decr_count()
items.decr_count(2)
assert items.size() == 2
items.erase_all()
assert items.size() == 0
assert probe.const_storage_agrees["int*"](items)

owner_vector_type = sc.sc_pvector["sc_core_clock_vector_probe::Owner*"]
owners = owner_vector_type()
owners[0] = probe.owner_pointer(0)
owners[2] = probe.owner_pointer(2)
owners.put(probe.owner_pointer(1), 1)
owner_copy = owner_vector_type(owners)
assert tuple(probe.owner_value(probe.owner_pointer(index)) for index in range(3)) == (17, 4, 29)
assert tuple(probe.fetched_owner_value(owners, index) for index in range(3)) == (17, 4, 29)
assert probe.owner_vector_value(owner_copy, 1) == 4
assert probe.owner_vector_points_to_fixture(owner_copy, 1, 1)
assert probe.owner_vector_value(owner_copy, 0) == 17 and probe.owner_vector_value(owner_copy, 2) == 29
assert probe.iterator_storage_agrees["sc_core_clock_vector_probe::Owner*"](owners)

item_vector_type = sc.sc_pvector["sc_core_clock_vector_probe::Item*"]
typed_items = item_vector_type()
typed_items[0] = probe.item_pointer(0)
typed_items[2] = probe.item_pointer(1)
assert typed_items.size() == 3 and not bool(typed_items[1])
assert probe.item_vector_value(typed_items, 0) == 11
assert probe.item_vector_points_to_fixture(typed_items, 2, 1)
typed_items[1] = probe.item_pointer(2)
assert probe.item_vector_value(typed_items, 1) == 19

time_vector_type = sc.sc_pvector["sc_core::sc_time*"]
times = time_vector_type()
for index in range(3):
    times.push_back(probe.time_pointer(index))
time_assigned = time_vector_type()
probe.assign["sc_core::sc_time*"](time_assigned, times)
time_values = tuple(probe.fetched_time_nanoseconds(time_assigned, index) for index in range(3))
assert time_values == (3.0, 1.0, 2.0)
assert probe.converted_size["sc_core::sc_time*"](time_assigned) == 3

gc.collect()
print(
    "SC_CORE_CLOCK_VECTOR "
    f"clock={time_clock.period().to_seconds() * 1e9:.0f},{time_clock.duty_cycle():.2f},"
    f"{time_clock.start_time().to_seconds() * 1e9:.0f},"
    f"{int(time_clock.posedge_first())},unit_default=6,0.50,0,edges="
    f"{int(default_clock.read())}{int(time_clock.read())}"
    f"{int(short_clock.read())}{int(long_clock.read())}{int(legacy_clock.read())},"
    f"time={sc.sc_time_stamp().to_seconds() * 1e9:.0f},reports={','.join(map(str, report_ids))},"
    f"protected={int(protected_clock)} "
    f"vector={','.join(map(str, sorted_values))} owners=17,4,29 items=11,19 "
    f"times=3,1,2 "
    f"copied={probe.converted_size['int*'](copied)} callback={len(comparison_pairs)}"
)
