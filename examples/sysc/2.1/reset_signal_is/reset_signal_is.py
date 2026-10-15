"""Port of Accellera SystemC 2.3.4 reset_signal_is (Apache-2.0).

See the upstream LICENSE and NOTICE. The native clock and signal topology and
100-cycle active-low synchronous reset stimulus follow the C++ example.

Original source: examples/sysc/2.1/reset_signal_is/reset_signal_is.cpp
"""

from amal.eda.systemc import main, sc_core as sc


class Consumer(sc.sc_module):
    """Clocked consumer with active-low synchronous reset."""

    def __init__(self, name):
        """Register native clock/reset topology within a live module-name scope."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.m_clk = sc.sc_in_clk("m_clk")
        self.m_ready = sc.sc_inout[bool]("m_ready")
        self.m_reset = sc.sc_in[bool]("m_reset")
        self.m_valid = sc.sc_in[bool]("m_valid")
        self.m_value = sc.sc_in[int]("m_value")
        self.sc_cthread("consumer", self.consumer, self.m_clk.pos())
        self.reset_signal_is(self.m_reset, False)

    def consumer(self):
        """Print reset and received values at native clock timestamps."""
        print(f"{sc.sc_time_stamp()}: reset")
        while True:
            self.m_ready.write(True)
            sc.wait()
            while not self.m_valid.read():
                sc.wait()
            print(f"{sc.sc_time_stamp()}: {self.m_value.read()}")


class Producer(sc.sc_module):
    """Clocked producer advancing values when the consumer is ready."""

    def __init__(self, name):
        """Register native clock/reset topology within a live module-name scope."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.m_clk = sc.sc_in_clk("m_clk")
        self.m_ready = sc.sc_in[bool]("m_ready")
        self.m_reset = sc.sc_in[bool]("m_reset")
        self.m_valid = sc.sc_inout[bool]("m_valid")
        self.m_value = sc.sc_inout[int]("m_value")
        self.sc_cthread("producer", self.producer, self.m_clk.pos())
        self.reset_signal_is(self.m_reset, False)

    def producer(self):
        """Restart at zero on reset, matching the native process entry."""
        self.m_value.write(0)
        while True:
            self.m_valid.write(True)
            sc.wait()
            while not self.m_ready.read():
                sc.wait()
            self.m_value.write(self.m_value.read() + 1)


class Testbench(sc.sc_module):
    """Connect producer/consumer and drive the upstream reset sequence."""

    def __init__(self, name):
        """Register native clock/reset topology within a live module-name scope."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.m_clk = sc.sc_in_clk("m_clk")
        self.m_ready = sc.sc_signal[bool]("m_ready")
        self.m_reset = sc.sc_signal[bool]("m_reset")
        self.m_valid = sc.sc_signal[bool]("m_valid")
        self.m_value = sc.sc_signal[int]("m_value")
        self.m_consumer = Consumer("consumer")
        self.m_producer = Producer("producer")
        for module in (self.m_consumer, self.m_producer):
            for name in ("m_clk", "m_ready", "m_reset", "m_valid", "m_value"):
                getattr(module, name)(getattr(self, name))
        self.sc_cthread("testbench", self.testbench, self.m_clk.pos())

    def testbench(self):
        """Drive 100 cycles and stop; reset is low every twentieth cycle."""
        for state in range(100):
            self.m_reset.write(state % 20 != 12)
            sc.wait()
        sc.sc_stop()


def sc_main(args):
    """Run the original 5000-nanosecond bounded simulation."""
    clock = sc.sc_clock("clock")
    testbench = Testbench("testbench")
    testbench.m_clk(clock)
    sc.sc_start(5000, sc.SC_NS)
    print("Done")
    return 0


if __name__ == "__main__":
    main()
