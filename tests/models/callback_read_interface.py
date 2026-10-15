"""Validate Python computed values through stable native signal references."""

from amal.eda.systemc import sc_core as sc

module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
state = {"value": 17}
channel = sc.read_interface(int, "computed", lambda: state["value"])
port = sc.sc_in[int]("input")
port.bind(channel)
seen = []
model.sc_method("observe", lambda: seen.append(port.read()))
model.sensitive(port)
model.dont_initialize()
del module_name
channel.notify_changed()
sc.sc_start(1, sc.SC_NS)
assert seen == [17], seen
state["value"] = 23
channel.notify_changed()
sc.sc_start(1, sc.SC_NS)
assert seen == [17, 23], seen
assert port.read() == 23 and channel.get_data_ref() == 23
state["value"] = 29
channel.notify_changed()
assert port.read() == 23, "sample became visible before native update"
sc.sc_start(1, sc.SC_NS)
assert seen == [17, 23, 29]
channel.notify_changed()
sc.sc_start(1, sc.SC_NS)
assert seen == [17, 23, 29], "unchanged sample notified"
sc.sc_stop()
print("CALLBACK_READ_INTERFACE_OK")
