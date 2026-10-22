"""Exercise native event-list constructors, unions, copies and borrowed events."""

from amal.eda.systemc import sc_core as sc
import cppyy

events = [sc.sc_event(name) for name in ("a", "b", "c")]
counts = []
for cls, combine in ((sc.sc_event_or_list, "or"), (sc.sc_event_and_list, "and")):
    empty = cls()
    assert empty.empty() and empty.size() == 0
    first = cls(events[0])
    second = cls(events[1])
    if combine == "or":
        first |= events[0]
        first |= second
        first |= second
        expression_event = first | events[2]
        expression_list = first | second
    else:
        first &= events[0]
        first &= second
        first &= second
        expression_event = first & events[2]
        expression_list = first & second
    assert first.size() == 2
    expanded = cls(expression_event)
    repeated = cls(expression_list)
    assert expanded.size() == 3 and repeated.size() == 2
    copied = cls(first)
    assert copied.size() == 2
    empty.swap(expanded)
    assert empty.size() == 3 and expanded.empty()
    # Base views borrow the derived list; its owner stays alive throughout.
    base = cppyy.bind_object(cppyy.addressof(copied), sc.sc_event_list)
    source_base = cppyy.bind_object(cppyy.addressof(second), sc.sc_event_list)
    assert base.size() == 2 and not base.empty()
    base.__assign__(source_base)
    assert copied.size() == 1
    base.__assign__(base)
    assert copied.size() == 1
    counts.extend((first.size(), empty.size(), expanded.size(), repeated.size(), copied.size()))
print("EVENT_LIST_REFERENCE", *counts)
