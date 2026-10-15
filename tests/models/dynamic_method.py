"""Verify dynamic method event-expression and timed sensitivity."""

from amal.eda.systemc import sc_core as sc

first = sc.sc_event("first")
second = sc.sc_event("second")
seen = []
module_name = sc.sc_module_name("dynamic")
model = sc.sc_module(module_name)
del module_name


def tick():
    """Switch from initialization to OR, AND and timed sensitivity."""
    seen.append(str(sc.sc_time_stamp()))
    if len(seen) == 1:
        sc.next_trigger(first | second)
    elif len(seen) == 2:
        sc.next_trigger(first & second)
    elif len(seen) == 3:
        sc.next_trigger(sc.sc_time(2, sc.SC_NS))


model.sc_method("tick", tick)
sc.sc_start(1, sc.SC_NS)
first.notify()
sc.sc_start(1, sc.SC_NS)
first.notify()
sc.sc_start(1, sc.SC_NS)
second.notify()
sc.sc_start(3, sc.SC_NS)
assert seen == ["0 s", "1 ns", "3 ns", "5 ns"], seen
sc.sc_stop()
print("DYNAMIC_METHOD_OK")
