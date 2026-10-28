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
    extra_list = cls(events[2])
    other_type = sc.sc_event_and_list if combine == "or" else sc.sc_event_or_list
    wrong_list = other_type(events[2])
    for right in (events[2], extra_list):
        initial = first | events[0] if combine == "or" else first & events[0]
        chained = initial | right if combine == "or" else initial & right
        result = cls(chained)
        assert result.size() == 3
        # A successful append transferred the automatic list out of initial.
        initial.__destruct__()
        chained.__destruct__()
        assert result.size() == 3 and first.size() == 2
    invalid = first | events[0] if combine == "or" else first & events[0]
    try:
        if combine == "or":
            invalid | wrong_list
        else:
            invalid & wrong_list
    except TypeError:
        pass
    else:
        raise AssertionError("mixed event-list kinds were accepted")
    assert cls(invalid).size() == 2
    invalid.__destruct__()
    expression_event.push_back(events[2])
    expression_event.push_back(second)
    expression_type = sc.sc_event_expr[cls]
    alias = sc.sc_event_or_expr if combine == "or" else sc.sc_event_and_expr
    assert alias is expression_type
    # Discarding an unconsumed expression frees only its automatic list.
    discarded = first | events[2] if combine == "or" else first & events[2]
    discarded.__destruct__()
    assert first.size() == 2 and events[2].name() == "c"
    moved_expression = expression_type(expression_event)
    expanded = cls(moved_expression)
    moved_expression.__destruct__()
    expression_event.__destruct__()
    # release() returns a temporary borrowed list; its copy consumes storage.
    released = expression_list.release()
    assert released.size() == 2
    repeated = cls(released)
    # The borrowed proxy is now invalid: do not access or destroy it.
    del released
    expression_list.__destruct__()
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
