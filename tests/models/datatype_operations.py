"""Check integer overflow, mutable bits/ranges and typed port propagation."""

from amal.eda.systemc import sc_core as sc, sc_dt as dt


class Ports(sc.sc_module):
    """Retain arbitrary-width input/output ports and connected native signals."""

    def __init__(self, name, value_type):
        """Register a native-style combinational copy for one runtime type."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.source = sc.sc_signal[value_type]("source")
        self.target = sc.sc_signal[value_type]("target")
        self.input = sc.sc_in[value_type]("input")
        self.output = sc.sc_out[value_type]("output")
        self.input.bind(self.source)
        self.output.bind(self.target)
        self.sc_method("copy", self.copy)
        self.sensitive(self.input)

    def copy(self):
        """Copy the input using native deferred output scheduling."""
        self.output.write(self.input.read())


models = []
for kind, widths in (
    (dt.sc_uint, (1, 4, 17, 64)),
    (dt.sc_int, (4, 17, 64)),
    (dt.sc_biguint, (65, 137, 257)),
    (dt.sc_bigint, (65, 137, 257)),
):
    for width in widths:
        value = kind[width](0)
        value.__assign__(kind[width]("0b" + "1" * width))
        assert value.range(3 if width > 3 else 0, 0).to_uint() == (15 if width > 3 else 1)
        value[0].__assign__(False)
        assert not value[0].to_bool()
        value.range(min(width - 1, 3), 0).__assign__(kind[width](1))
        assert value[0].to_bool()
        assert value == kind[width](value)
        overflow = kind[width]("0b1" + "0" * width)
        assert overflow.to_uint64() == 0
        if kind in (dt.sc_int, dt.sc_bigint):
            assert kind[width](-1).to_int64() == -1
        model = Ports(f"ports_{len(models)}", kind[width])
        model.source.write(value)
        models.append((model, kind[width](value)))
sc.sc_start(1, sc.SC_NS)
for model, expected in models:
    assert model.target.read() == expected
assert sc.sc_time_stamp() == sc.sc_time(1, sc.SC_NS)
sc.sc_stop()
print("DATATYPE_OPERATIONS_OK")
