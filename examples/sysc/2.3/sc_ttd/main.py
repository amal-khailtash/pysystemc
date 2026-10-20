"""Port Accellera sysc/2.3/sc_ttd/main.cpp and include/sc_ttd.h.

Apache-2.0; see upstream LICENSE/NOTICE. Native typed signals and ports retain
channel storage until simulation ends. Blocking protocol operations use guarded
Python cycle waits so synchronous reset preserves scheduler unwind.
"""

from amal.eda.systemc import main, sc_core as sc, sc_dt as dt

DATA = dt.sc_uint[8]


class Channel:
    """Own native data and the two handshake signals for one connection."""

    def __init__(self, name, value_type):
        """Create signals of the supplied native-valid datatype."""
        self.data = sc.sc_signal[value_type](name + "_data")
        self.first = sc.sc_signal[bool](name + "_read_toggle")
        self.second = sc.sc_signal[bool](name + "_write_toggle")


class Input:
    """Read-side ports retaining the native handshake direction."""

    def __init__(self, name, value_type):
        """Create ports; bind before elaboration and retain through simulation."""
        self.value_type = value_type
        self.data = sc.sc_in[value_type](name + "_data")
        self.first = sc.sc_out[bool](name + "_read_toggle")
        self.second = sc.sc_in[bool](name + "_write_toggle")

    def bind(self, channel):
        """Bind each port to its matching channel signal."""
        for name in ("data", "first", "second"):
            getattr(self, name).bind(getattr(channel, name))

    def reset(self):
        """Restore the read-side handshake bit from a clocked process."""
        self.first.write(False)

    def read(self):
        """Wait at least one cycle and return a copied native data value.

        Call only from a clocked Python process. Scheduler unwind propagates.
        """
        while True:
            sc.wait()
            if self.second.read() != self.first.read():
                break
        self.first.write(not self.first.read())
        return self.value_type(self.data.read())


class Output:
    """Write-side ports retaining the native handshake direction."""

    def __init__(self, name, value_type):
        """Create ports to be bound before elaboration."""
        self.data = sc.sc_out[value_type](name + "_data")
        self.first = sc.sc_in[bool](name + "_read_toggle")
        self.second = sc.sc_out[bool](name + "_write_toggle")

    def bind(self, channel):
        """Bind each port to its matching channel signal."""
        for name in ("data", "first", "second"):
            getattr(self, name).bind(getattr(channel, name))

    def reset(self):
        """Restore the write-side handshake bit from a clocked process."""
        self.second.write(False)

    def write(self, value):
        """Transfer a native value with the upstream cycle handshake.

        Call only from a clocked Python process. Scheduler unwind propagates.
        """
        while True:
            sc.wait()
            if self.second.read() == self.first.read():
                break
        self.data.write(value)
        self.second.write(not self.second.read())


class DUT(sc.sc_module):
    """Buffer successively larger batches exactly as in the native example."""

    def __init__(self, name):
        """Own ports and register an active-low synchronous reset process."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_in[bool]("clock")
        self.reset = sc.sc_in[bool]("reset")
        self.input = Input("input", DATA)
        self.output = Output("output", DATA)
        self.sc_cthread("thread", self.run, self.clock.pos())
        self.reset_signal_is(self.reset, False)

    def run(self):
        """Reset protocol bits, then copy input batches to the output."""
        self.input.reset()
        self.output.reset()
        sc.wait()
        while True:
            for size in range(10):
                values = []
                for _ in range(size):
                    value = self.input.read()
                    values.append(value)
                    print(f"         {int(value):3}          {sc.sc_time_stamp()}")
                for value in values:
                    self.output.write(value)


class TB(sc.sc_module):
    """Produce increasing values and stop after forty consumed values."""

    def __init__(self, name):
        """Register consumer before producer, preserving native process order."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_in[bool]("clock")
        self.reset = sc.sc_in[bool]("reset")
        self.input = Input("input", DATA)
        self.output = Output("output", DATA)
        self.consumed = []
        for name, callback in (("consumer", self.consume), ("producer", self.produce)):
            self.sc_cthread(name, callback, self.clock.pos())
            self.reset_signal_is(self.reset, False)

    def consume(self):
        """Read forty values, record their order and stop the native kernel."""
        self.input.reset()
        sc.wait()
        for _ in range(40):
            value = self.input.read()
            self.consumed.append(int(value))
            print(f"               {int(value):3}    {sc.sc_time_stamp()}")
        sc.sc_stop()

    def produce(self):
        """Apply the native producer's increasing cycle pauses every six values."""
        self.output.reset()
        sc.wait()
        index = 0
        while True:
            print(f"  {index:3}                 {sc.sc_time_stamp()}")
            self.output.write(DATA(index))
            if index and index % 6 == 0:
                sc.wait(index)
            index += 1


def sc_main(args):
    """Run the fixed upstream stimulus with owned channels until native stop.

    Args:
        args: Entry arguments, unused by the upstream scenario.
    Returns:
        Zero after all forty consumer values are verified.
    """
    clock = sc.sc_clock("clock")
    dut = DUT("dut")
    dut_to_tb = Channel("dut_to_tb", DATA)
    reset = sc.sc_signal[bool]("reset")
    tb = TB("tb")
    tb_to_dut = Channel("tb_to_dut", DATA)
    for model in (dut, tb):
        model.clock.bind(clock)
        model.reset.bind(reset)
    dut.input.bind(tb_to_dut)
    dut.output.bind(dut_to_tb)
    tb.input.bind(dut_to_tb)
    tb.output.bind(tb_to_dut)
    print("producer dut consumer ")
    reset.write(False)
    sc.sc_start(1, sc.SC_NS)
    reset.write(True)
    sc.sc_start()
    assert tb.consumed == list(range(40)), tb.consumed
    assert sc.sc_end_of_simulation_invoked()
    print("Program completed")
    return 0


if __name__ == "__main__":
    main()
