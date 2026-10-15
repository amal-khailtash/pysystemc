"""Verify configured dynamic methods and zero-argument static thread waits."""

from amal.eda.systemc import sc_core as sc

clock = sc.sc_signal[bool]("clock")
seen = []
module_name = sc.sc_module_name("process_options")
model = sc.sc_module(module_name)


def thread():
    """Resume on the next statically sensitive clock event."""
    seen.append("start")
    sc.wait()
    seen.append("resumed")


model.sc_thread("thread", thread)
model.sensitive(clock)
del module_name
options = sc.sc_spawn_options()
options.spawn_method()
options.dont_initialize()
options.set_sensitivity(clock.value_changed_event())
handle = sc.sc_spawn(lambda: seen.append("dynamic"), "dynamic", options)
sc.sc_start(1, sc.SC_NS)
assert seen == ["start"], seen
clock.write(True)
sc.sc_start(1, sc.SC_NS)
assert seen.count("resumed") == seen.count("dynamic") == 1, seen
print("PROCESS_OPTIONS_OK")
