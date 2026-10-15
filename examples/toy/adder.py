"""Original parameterized adder inspired by ASIC World's adder tutorial.

Original example: https://www.asic-world.com/examples/systemc/adder.html#Adder
This implementation is original Python code illustrating the same behavior.
"""

from amal.eda.systemc import main, sc_core as sc, sc_dt as dt


class Adder(sc.sc_module):
    """Combinational unsigned adder with native width-limited output."""

    def __init__(self, name: str, width: int):
        """Create ports of the requested native sc_uint width."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.word = dt.sc_uint[width]
        self.a = sc.sc_in[self.word]("a")
        self.b = sc.sc_in[self.word]("b")
        self.sum = sc.sc_out[self.word]("sum")
        self.sc_method("add", self.add)
        self.sensitive(self.a)
        self.sensitive(self.b)

    def add(self):
        """Write the native-width sum after either input changes."""
        self.sum.write(self.word(self.a.read().to_uint64() + self.b.read().to_uint64()))


def sc_main(args: list[str]) -> int:
    """Verify ordinary sums and modulo overflow for 8- and 17-bit adders."""
    models = []
    for width in (8, 17):
        word = dt.sc_uint[width]
        signals = [sc.sc_signal[word](f"{label}_{width}") for label in ("a", "b", "sum")]
        dut = Adder(f"adder_{width}", width)
        dut.a(signals[0])
        dut.b(signals[1])
        dut.sum(signals[2])
        models.append((width, word, dut, signals))
    for left, right in ((0, 0), (3, 5), (255, 1), (131071, 1)):
        for width, word, dut, signals in models:
            signals[0].write(word(left))
            signals[1].write(word(right))
        sc.sc_start(1, sc.SC_NS)
        for width, word, dut, signals in models:
            assert signals[2].read().to_uint64() == (left + right) % (1 << width)
    print("ADDER_OK: native 8/17-bit sums and overflow")
    return 0


if __name__ == "__main__":
    main()
