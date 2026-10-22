"""Exercise native event-list constructors, unions, copies and borrowed events."""

from amal.eda.systemc import sc_core as sc

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
    copied.__assign__(second)
    assert copied.size() == 1
    copied.__assign__(copied)
    assert copied.size() == 1
    counts.extend((first.size(), empty.size(), expanded.size(), repeated.size(), copied.size()))
print("EVENT_LIST_REFERENCE", *counts)
