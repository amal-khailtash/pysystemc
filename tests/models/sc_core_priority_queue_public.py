"""Exercise native sc_ppq families through retained Python and C++ comparators."""

from amal.eda.systemc import sc_core as sc
import cppyy
import gc
import pathlib
import threading
import weakref

cppyy.include(str(pathlib.Path(__file__).resolve().parents[1] / "native" / "sc_core_priority_queue_probe.h"))
probe = cppyy.gbl.sc_core_priority_queue_probe


def build_python_item_queue():
    """Create a typed queue whose comparator owner must follow queue lifetime."""
    comparisons = []
    callback_threads = []

    def compare(left, right):
        """Rank borrowed Item pointers by key while the caller holds the GIL."""
        lhs = probe.read_item_key(left)
        rhs = probe.read_item_key(right)
        comparisons.append((lhs, rhs))
        callback_threads.append(threading.get_ident())
        return (lhs > rhs) - (lhs < rhs)

    callback_ref = weakref.ref(compare)
    queue_type = sc.sc_ppq["sc_core_priority_queue_probe::Item*"]
    queue = queue_type(1, compare)
    return queue, callback_ref, comparisons, callback_threads


base = probe.BaseProbe(1, probe.compare_ints)
assert base.empty() and base.size() == 0
assert (base.parent_index(7), base.left_index(4), base.right_index(4)) == (3, 8, 9)
for index in range(5):
    base.insert(probe.integer_pointer(index))
assert not base.empty() and base.size() == 5
assert probe.read_integer(base.top()) == 5
base.repair(1)
assert probe.read_integer(base.top()) == 5
base_expected = (5, 4, 3, 1, 1)
for size, expected in zip(range(4, -1, -1), base_expected, strict=True):
    assert not base.empty()
    assert probe.read_integer(base.extract_top()) == expected
    assert base.size() == size
assert base.empty() and base.size() == 0
del base

typed_queue, comparator_ref, comparisons, callback_threads = build_python_item_queue()
gc.collect()
assert comparator_ref() is not None
assert typed_queue.empty() and typed_queue.size() == 0
for index in range(5):
    typed_queue.insert(probe.item_pointer(index))
assert not typed_queue.empty() and typed_queue.size() == 5
assert probe.read_item_key(typed_queue.top()) == 8
assert comparisons and set(callback_threads) == {threading.get_ident()}
typed_expected = (8, 8, 5, 3, 2)
for size, expected in zip(range(4, -1, -1), typed_expected, strict=True):
    assert not typed_queue.empty()
    value = typed_queue.extract_top()
    assert probe.read_item_key(value) == expected
    assert typed_queue.size() == size
assert typed_queue.empty()
assert tuple(probe.item_pointer(index).key for index in range(5)) == (8, 3, 8, 2, 5)
del typed_queue
gc.collect()
assert comparator_ref() is None

time_queue = sc.sc_ppq["sc_core::sc_time*"](2, probe.compare_times)
for index in range(4):
    time_queue.insert(probe.time_pointer(index))
assert probe.time_nanoseconds(time_queue.top()) == 1.0
time_expected = (1.0, 2.0, 2.0, 4.0)
for size, expected in zip(range(3, -1, -1), time_expected, strict=True):
    value = time_queue.extract_top()
    assert probe.time_nanoseconds(value) == expected
    assert time_queue.size() == size
assert time_queue.empty()
del time_queue

timed_queue = sc.sc_ppq["sc_core::sc_event_timed*"](2, probe.compare_null_timed_events)
null_event = probe.null_timed_event()
timed_queue.insert(null_event)
timed_queue.insert(null_event)
assert timed_queue.size() == 2 and probe.timed_event_is_null(timed_queue.top())
assert probe.timed_event_is_null(timed_queue.extract_top())
assert probe.timed_event_is_null(timed_queue.extract_top())
assert timed_queue.empty()
del timed_queue

print("SC_PPQ_REFERENCE base=5,4,3,1,1 typed=8,8,5,3,2 time=1,2,2,4 event_timed=null protected=3,8,9 borrowed=1")
