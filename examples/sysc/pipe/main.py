"""Port Accellera sysc/pipe main and stage processes (Apache-2.0).

See upstream LICENSE/NOTICE. Native double ports preserve deferred updates;
each stage is a separate noninitializing method on the manually driven clock.
"""

import math
from amal.eda.systemc import sc_core as sc


class Stage(sc.sc_module):
    """Own directional native ports and one positive-edge callback."""

    def __init__(self, name, inputs, outputs, callback):
        """Bind supplied signal lists and retain the callback through simulation."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.inputs = [sc.sc_in["double"](f"in_{i}") for i in range(len(inputs))]
        self.outputs = [sc.sc_out["double"](f"out_{i}") for i in range(len(outputs))]
        self.clock = sc.sc_in[bool]("clock")
        for port, signal in zip(self.inputs, inputs):
            port.bind(signal)
        for port, signal in zip(self.outputs, outputs):
            port.bind(signal)
        self.callback = callback
        self.sc_method("process", self.run)
        self.sensitive(self.clock.pos())
        self.dont_initialize()

    def run(self):
        """Read preceding update values and schedule new stage outputs."""
        values = self.callback(*[port.read() for port in self.inputs])
        for port, value in zip(self.outputs, values):
            port.write(value)


def sc_main():
    """Run fifty native clock cycles and verify observed pipeline latency.

    Returns:
        None after fifty display callbacks and 1000 ns have been verified.
    """
    signals = [sc.sc_signal["double"](name) for name in ("in1", "in2", "sum", "diff", "prod", "quot", "powr")]
    clock = sc.sc_signal[bool]("clock")
    generated = [134.56, 98.24]
    records = []

    def generate():
        """Apply the upstream double decrements once per rising edge."""
        generated[0] -= 1.5
        generated[1] -= 2.8
        return generated

    def multiply(a, b):
        """Use the upstream zero-divisor replacement."""
        b = b if b else 5.0
        return a * b, a / b

    def display(value):
        """Retain time/value observations and reproduce native printf output."""
        records.append((sc.sc_time_stamp().to_string(), value))
        print(f"Result = {value:f}")
        return ()

    modules = [
        Stage("numgen", [], signals[:2], generate),
        Stage("stage1", signals[:2], signals[2:4], lambda a, b: (a + b, a - b)),
        Stage("stage2", signals[2:4], signals[4:6], multiply),
        Stage("stage3", signals[4:6], signals[6:], lambda a, b: (math.pow(a, b) if a > 0 and b > 0 else 0.0,)),
        Stage("display", signals[6:], [], display),
    ]
    for module in modules:
        module.clock.bind(clock)
    sc.sc_start(0, sc.SC_NS)
    for _ in range(50):
        clock.write(True)
        sc.sc_start(10, sc.SC_NS)
        clock.write(False)
        sc.sc_start(10, sc.SC_NS)
    assert len(records) == 50
    assert [time for time, _ in records] == ["0 s", *[f"{20 * i} ns" for i in range(1, 50)]]
    assert all(value == 0 for _, value in records[:4])
    assert sc.sc_time_stamp() == sc.sc_time(1000, sc.SC_NS)


if __name__ == "__main__":
    sc_main()
