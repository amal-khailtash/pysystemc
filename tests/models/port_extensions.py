"""Exercise FIFO ports, resolved ports and runtime signal writer policies."""

from amal.eda.systemc import sc_core as sc, sc_dt as dt


sc.sc_fifo_out[int].write.__release_gil__ = True
sc.sc_fifo_in[int].read.__release_gil__ = True


class Model(sc.sc_module):
    """Retain native interfaces and ports while Python processes use them."""

    def __init__(self, name):
        """Bind FIFO and resolved ports and register bounded producer/consumer."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.fifo = sc.sc_fifo[int]("fifo", 1)
        self.input = sc.sc_fifo_in[int]("input")
        self.output = sc.sc_fifo_out[int]("output")
        self.input.bind(self.fifo)
        self.output.bind(self.fifo)
        self.resolved = sc.sc_signal_resolved("resolved")
        self.logic_input = sc.sc_in_resolved("logic_input")
        self.logic_output = sc.sc_out_resolved("logic_output")
        self.logic_input.bind(self.resolved)
        self.logic_output.bind(self.resolved)
        self.values = []
        self.sc_thread("produce", self.produce)
        self.sc_thread("consume", self.consume)

    def produce(self):
        """Send ordered integers through a capacity-one FIFO and drive logic."""
        self.logic_output.write(dt.sc_logic("1"))
        for value in (17, 23, 41):
            self.output.write(value)

    def consume(self):
        """Collect all blocking port reads and their simulation timestamps."""
        for _ in range(3):
            self.values.append(self.input.read())
            sc.wait(1, sc.SC_NS)


model = Model("ports")
policies = []
for policy in (sc.SC_ONE_WRITER, sc.SC_MANY_WRITERS, sc.SC_UNCHECKED_WRITERS):
    signal = sc.sc_signal[int, policy](f"policy_{int(policy)}")
    signal.write(17)
    policies.append(signal)
sc.sc_start(4, sc.SC_NS)
assert model.values == [17, 23, 41], model.values
assert model.logic_input.read().to_char() == "1"
assert model.fifo.num_available() == 0
assert model.fifo.num_free() == 1
assert [signal.read() for signal in policies] == [17, 17, 17]
sc.sc_stop()
print("PORT_EXTENSIONS_OK")
