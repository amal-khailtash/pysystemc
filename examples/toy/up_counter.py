"""Original 8-bit counter inspired by ASIC World's up-counter tutorial.

Original example: https://www.asic-world.com/examples/systemc/counters.html#8-Bit_Up_Counter
This implementation is original Python code illustrating the same behavior.
"""

from amal.eda.systemc import main, sc_core as sc, sc_dt as dt


class UpCounter(sc.sc_module):
    """Positive-edge counter with synchronous reset and enable."""

    def __init__(self, name: str):
        """Register a method sensitive only to the clock's positive edge."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clk = sc.sc_in[bool]("clk")
        self.reset = sc.sc_in[bool]("reset")
        self.enable = sc.sc_in[bool]("enable")
        self.out = sc.sc_out[dt.sc_uint[8]]("out")
        self.count = dt.sc_uint[8](0)
        self.sc_method("counter", self.counter)
        self.sensitive(self.clk.pos())
        self.dont_initialize()

    def counter(self):
        """Reset, increment or hold the native eight-bit count."""
        if self.reset.read():
            self.count = dt.sc_uint[8](0)
        elif self.enable.read():
            self.count = dt.sc_uint[8](self.count.to_uint() + 1)
        self.out.write(self.count)


def sc_main(args: list[str]) -> int:
    """Check reset, disabled hold, positive edges and wrap after 256 increments."""
    clk, reset, enable = [sc.sc_signal[bool](name) for name in ("clk", "reset", "enable")]
    out = sc.sc_signal[dt.sc_uint[8]]("out")
    dut = UpCounter("counter")
    dut.clk(clk)
    dut.reset(reset)
    dut.enable(enable)
    dut.out(out)

    def tick():
        """Drive a complete clock cycle with separate signal update phases."""
        clk.write(False)
        sc.sc_start(1, sc.SC_NS)
        clk.write(True)
        sc.sc_start(1, sc.SC_NS)

    reset.write(True)
    enable.write(True)
    tick()
    assert out.read().to_uint() == 0
    reset.write(False)
    for expected in range(1, 258):
        tick()
        assert out.read().to_uint() == expected % 256
    enable.write(False)
    tick()
    assert out.read().to_uint() == 1
    reset.write(True)
    tick()
    assert out.read().to_uint() == 0
    print("UP_COUNTER_OK: reset, enable, edges and 8-bit wrap")
    return 0


if __name__ == "__main__":
    main()
