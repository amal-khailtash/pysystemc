"""Validate native update semantics, ports, arbitrary datatypes and VCD storage."""

from pathlib import Path
from amal.eda.systemc import sc_core as sc, sc_dt as dt


class Observer(sc.sc_module):
    """Retain ports and record signal versus buffer update counts."""

    def __init__(self, name, signal, buffer):
        """Bind input and inout ports to caller-owned native channels."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.input = sc.sc_in[bool]("input")
        self.io = sc.sc_inout[bool]("io")
        self.input.bind(signal)
        self.io.bind(buffer)
        self.counts = [0, 0]
        self.sc_method("signal", self.signal_tick)
        self.sensitive(self.input)
        self.dont_initialize()
        self.sc_method("buffer", self.buffer_tick)
        self.sensitive(self.io)
        self.dont_initialize()

    def signal_tick(self):
        """Count value changes observed through an input port."""
        self.counts[0] += 1

    def buffer_tick(self):
        """Count buffer updates, including repeated identical writes."""
        self.counts[1] += 1


signal = sc.sc_signal[bool]("signal")
buffer = sc.sc_buffer[bool]("buffer")
model = Observer("observer", signal, buffer)
channels = []
for kind, width in ((dt.sc_uint, 17), (dt.sc_int, 17), (dt.sc_biguint, 137), (dt.sc_bigint, 137)):
    value = kind[width](7)
    value.__assign__(kind[width](9))
    assert value == kind[width](9)
    assert value > kind[width](8)
    assert value.range(3, 0).to_uint() == 9
    channel = sc.sc_signal[kind[width]](f"value_{len(channels)}")
    channels.append((channel, value))
bits = sc.sc_signal[dt.sc_bv[137]]("bits")
logic = sc.sc_signal[dt.sc_lv[17]]("logic")
with sc.vcd_trace("matrix", time_unit=(1, sc.SC_NS)) as trace:
    sc.sc_trace(trace, signal, "boolean")
    for index, (channel, value) in enumerate(channels):
        sc.sc_trace(trace, channel, f"integer_{index}")
    sc.sc_trace(trace, bits, "bits137")
    sc.sc_trace(trace, logic, "logic17")
    sc.sc_start(1, sc.SC_NS)
    for channel, value in channels:
        channel.write(value)
        assert channel.read() != value, "write became visible before native update"
    bits.write(dt.sc_bv[137]("0" * 133 + "1010"))
    logic.write(dt.sc_lv[17]("01XZ"))
    signal.write(True)
    buffer.write(True)
    sc.sc_start(1, sc.SC_NS)
    assert model.input.read() and model.io.read()
    assert bits.read().to_string() == "0" * 133 + "1010"
    assert model.counts == [1, 1]
    for channel, value in channels:
        assert channel.read() == value
    signal.write(True)
    buffer.write(True)
    sc.sc_start(1, sc.SC_NS)
    assert model.counts == [1, 2], model.counts
    signal.write(False)
    sc.sc_start(1, sc.SC_NS)
    assert model.counts == [2, 2]
text = Path("matrix.vcd").read_text()
variables = {
    line.split()[4]: (int(line.split()[2]), line.split()[3]) for line in text.splitlines() if line.startswith("$var")
}
assert variables["bits137"][0] == 137
assert variables["logic17"][0] == 17
for index, width in enumerate((17, 17, 137, 137)):
    assert variables[f"integer_{index}"][0] == width
    assert f"b1001 {variables[f'integer_{index}'][1]}" in text
bit_values = [
    line.split()[0][1:]
    for line in text.splitlines()
    if line.startswith("b") and line.endswith(" " + variables["bits137"][1])
]
assert any(int(value, 2) == 10 for value in bit_values), bit_values
assert "xz" in text.lower()
assert "#1" in text and "#3" in text
sc.sc_stop()
print(f"MATRIX_RECORD {model.counts[0]}|{model.counts[1]}|{sc.sc_time_stamp()}")
print("MATRIX_VALUES " + "|".join(str(channel.read().to_int64()) for channel, _ in channels))
print("MODELING_MATRIX_OK")
