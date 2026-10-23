"""Compare native signal interface dispatch, edge semantics and channel ownership."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_core as sc, sc_dt as dt
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_signal_interfaces_probe.h"))
probe = cppyy.gbl.signal_interfaces_probe
owned = probe.owned_channel()
owned.__python_owns__ = True
assert owned.kind() == "sc_signal_channel"
assert cppyy.addressof(owned.default_event()) == cppyy.addressof(owned.value_changed_event())
del owned
gc.collect()
assert probe.destructions() == 1
print("SIGNAL_INTERFACE_DESTRUCTION 1")

boolean = probe.Interface[bool]("boolean")
logic = probe.Interface[dt.sc_logic]("logic")
wide_type = dt.sc_biguint[137]
wide = probe.Interface[wide_type]("wide")
channel = probe.Channel("channel")
inputs = [
    cppyy.bind_object(cppyy.addressof(value.input()), sc.sc_signal_in_if[typ])
    for value, typ in ((boolean, bool), (logic, dt.sc_logic), (wide, wide_type))
]
outputs = [
    cppyy.bind_object(cppyy.addressof(value.output()), sc.sc_signal_write_if[typ])
    for value, typ in ((boolean, bool), (logic, dt.sc_logic), (wide, wide_type))
]
combined = [value.inout() for value in (boolean, logic, wide)]
for value, interface, output in zip((boolean, logic, wide), inputs, outputs, strict=True):
    assert value.aliases()
    assert cppyy.addressof(interface.value_changed_event()) == cppyy.addressof(value.backing.value_changed_event())
    assert output.get_writer_policy() == probe.default_policy()
assert channel.kind() == "sc_signal_channel"
assert cppyy.addressof(channel.default_event()) == cppyy.addressof(channel.value_changed_event())
records = []


class Model(sc.sc_module):
    """Retain typed interface owners while native callbacks observe updates."""

    def __init__(self):
        """Register driver and change observer without initializing the observer."""
        name = sc.sc_module_name("interfaces")
        super().__init__(name)
        self.sc_thread("drive", self.drive)
        self.sc_method("observe", self.observe)
        for interface in inputs:
            self.sensitive(interface.value_changed_event())
        self.sensitive(channel.default_event())
        self.dont_initialize()

    def drive(self):
        """Write via output and combined interfaces at three distinct times."""
        for index, (bit, state) in enumerate(((True, "1"), (False, "X"), (True, "0")), 1):
            sc.wait(1, sc.SC_NS)
            destination = outputs if index != 2 else combined
            destination[0].write(bit)
            destination[1].write(dt.sc_logic(state))
            destination[2].write(wide_type(str((1 << 136) + index)))
            channel.change()
        sc.wait(1, sc.SC_NS)
        sc.sc_stop()

    def observe(self):
        """Compare native input virtual dispatch and bool/logic edge flags."""
        assert all(value.aliases() for value in (boolean, logic, wide))
        assert all(interface.event() for interface in inputs) and channel.event()
        assert inputs[0].read() == inputs[0].get_data_ref()
        assert inputs[1].read() == inputs[1].get_data_ref()
        assert inputs[2].read() == inputs[2].get_data_ref()
        for value, interface in ((boolean, inputs[0]), (logic, inputs[1])):
            assert cppyy.addressof(interface.posedge_event()) == cppyy.addressof(value.backing.posedge_event())
            assert cppyy.addressof(interface.negedge_event()) == cppyy.addressof(value.backing.negedge_event())
        row = (
            sc.sc_time_stamp().value(),
            sc.sc_delta_count(),
            int(inputs[0].read()),
            int(inputs[0].posedge()),
            int(inputs[0].negedge()),
            inputs[1].read().to_char(),
            int(inputs[1].posedge()),
            int(inputs[1].negedge()),
            inputs[2].read().to_string(),
        )
        records.append(row)
        print("SIGNAL_INTERFACE_RECORD", *row)


model = Model()
sc.sc_start()
assert len(records) == 3
assert [row[0] for row in records] == [1000, 2000, 3000]
assert [row[2:5] for row in records] == [(1, 1, 0), (0, 0, 1), (1, 1, 0)]
assert [row[5:8] for row in records] == [("1", 1, 0), ("X", 0, 0), ("0", 0, 1)]
assert not any(interface.event() for interface in inputs) and not channel.event()
print("SIGNAL_INTERFACE_FINAL 4000 3 1")
