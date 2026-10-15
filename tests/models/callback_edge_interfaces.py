"""Validate callback-sampled bool/logic native reference and edge interfaces."""

from amal.eda.systemc import sc_core as sc, sc_dt as dt

module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
state = {"bool": False, "logic": "X"}
boolean = sc.read_interface(bool, "boolean", lambda: state["bool"])
logic = sc.read_interface(dt.sc_logic, "logic", lambda: dt.sc_logic(state["logic"]))
bool_port = sc.sc_in[bool]("bool_port")
logic_port = sc.sc_in[dt.sc_logic]("logic_port")
bool_port.bind(boolean)
logic_port.bind(logic)
records = {"bool_pos": [], "bool_neg": [], "logic_pos": [], "logic_neg": []}


def observe(name):
    """Capture edge time after native update and assert interface edge status."""
    records[name].append(str(sc.sc_time_stamp()))
    port = bool_port if name.startswith("bool") else logic_port
    assert port.event()
    assert port.posedge() if name.endswith("pos") else port.negedge()


for name, port, positive in (
    ("bool_pos", bool_port, True),
    ("bool_neg", bool_port, False),
    ("logic_pos", logic_port, True),
    ("logic_neg", logic_port, False),
):
    model.sc_method(name, lambda name=name: observe(name))
    model.sensitive(port.pos() if positive else port.neg())
    model.dont_initialize()
del module_name
for value, bit in ((False, "X"), (True, "1"), (False, "0"), (False, "Z"), (True, "1")):
    state.update(bool=value, logic=bit)
    boolean.notify_changed()
    logic.notify_changed()
    sc.sc_start(1, sc.SC_NS)
assert records == {
    "bool_pos": ["1 ns", "4 ns"],
    "bool_neg": ["2 ns"],
    "logic_pos": ["1 ns", "4 ns"],
    "logic_neg": ["2 ns"],
}, records
assert bool_port.read() and logic_port.read().to_char() == "1"
sc.sc_stop()
print("CALLBACK_EDGE_INTERFACES_OK")
