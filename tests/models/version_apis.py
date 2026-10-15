"""Validate time, registry and RTTI APIs added across SystemC 2.3.x."""

from amal.eda.systemc import sc_core as sc

time = sc.sc_time.from_string("10 us")
assert time == sc.sc_time(10000, sc.SC_NS)
assert sc.sc_time.from_value(time.value()) == time
tuple_value = sc.sc_time_tuple(time)
assert tuple_value.value() == 10
assert tuple_value.unit() == sc.SC_US
assert tuple_value.unit_symbol() == "us"
assert sc.sc_time(15, sc.SC_NS) % sc.sc_time(4, sc.SC_NS) == sc.sc_time(3, sc.SC_NS)
try:
    time % sc.SC_ZERO_TIME
except ValueError:
    pass
else:
    raise AssertionError("zero time divisor accepted")
assert sc.sc_register_hierarchical_name("external.name")
assert sc.sc_hierarchical_name_exists("external.name")
assert sc.sc_get_hierarchical_name("external.name") == "external.name"
assert sc.sc_unregister_hierarchical_name("external.name")
assert not sc.sc_hierarchical_name_exists("external.name")
module_name = sc.sc_module_name("version")
model = sc.sc_module(module_name)
signal = sc.sc_signal[bool]("initialized", True)
port = sc.sc_in[bool]("input")
port.bind(signal)
assert port.get_interface_type() == port.get_interface_type()
assert signal.read()
seen = []
model.sc_method("observe", lambda: seen.append(True))
model.sensitive(signal)
model.dont_initialize()
del module_name
sc.sc_start(1, sc.SC_NS)
assert seen == [], "constructor initialization generated a value-change event"
options = sc.sc_spawn_options()
options.dont_initialize()
options.set_sensitivity(sc.sc_event.none)
handle = sc.sc_spawn(lambda: seen.append(False), "never", options)
assert handle.basename() == "never"
sc.sc_start(1, sc.SC_NS)
assert seen == []
sc.sc_stop()
print("VERSION_APIS_OK")
