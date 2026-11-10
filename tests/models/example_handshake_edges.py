"""Compare bounded handshake cases using the runnable Python port classes."""

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc

ROOT = Path(__file__).resolve().parents[2]
SCENARIOS = {"stalled_consumer", "delayed_ack", "reset_pending"}
PROTOCOLS = {"rvd", "ttd"}


def load_example(protocol):
    """Load the selected upstream port by its explicit path, not a module alias."""
    source = ROOT / "examples" / "sysc" / "2.3" / f"sc_{protocol}" / "main.py"
    spec = spec_from_file_location(f"pysystemc_{protocol}_edge_port", source)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load handshake example {source}")
    module = module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def record(tag, value=-1):
    """Emit a role/value/time record matching the native reference format."""
    nanoseconds = round(sc.sc_time_stamp().to_seconds() * 1_000_000_000)
    print(f"EDGE {tag} value={value} ns={nanoseconds}")


class Source(sc.sc_module):
    """Drive selected values through the example's real Output class."""

    def __init__(self, name, protocol, scenario, value_type, output_type):
        """Register a resettable clocked producer and retain endpoint state."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_in[bool]("clock")
        self.reset = sc.sc_in[bool]("reset")
        self.output = output_type("output", value_type)
        self.value_type = value_type
        self.toggle_protocol = protocol == "ttd"
        self.scenario = scenario
        self.epochs = 0
        self.sc_cthread("run", self.run, self.clock.pos())
        self.reset_signal_is(self.reset, False)

    def bind(self, clock, reset, channel):
        """Bind clock, native reset, and channel before simulation."""
        self.clock.bind(clock)
        self.reset.bind(reset)
        self.output.bind(channel)

    def transfer(self, value):
        """Log both entry to and completion of one blocking output call."""
        record("SEND_BEGIN", value)
        self.output.write(self.value_type(value))
        record("SEND_DONE", value)

    def run(self):
        """Execute the fixed producer sequence; reset restarts this callback."""
        self.output.reset()
        sc.wait()
        if self.scenario == "stalled_consumer":
            self.transfer(41)
        elif self.scenario == "delayed_ack":
            self.transfer(11)
            self.transfer(22)
        elif self.scenario == "reset_pending":
            if self.epochs == 0:
                self.epochs = 1
                self.transfer(41)
                # The reader has not consumed 41; this call remains pending.
                self.transfer(42)
            else:
                self.epochs = 2
                self.transfer(43 if self.toggle_protocol else 42)
        while True:
            sc.wait()


class Sink(sc.sc_module):
    """Apply clocked consumer delays then read through the example Input class."""

    def __init__(self, name, scenario, value_type, input_type):
        """Register the active-low-reset consumer process."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_in[bool]("clock")
        self.reset = sc.sc_in[bool]("reset")
        self.input = input_type("input", value_type)
        self.scenario = scenario
        self.sc_cthread("run", self.run, self.clock.pos())
        self.reset_signal_is(self.reset, False)

    def bind(self, clock, reset, channel):
        """Bind clock, native reset, and channel before simulation."""
        self.clock.bind(clock)
        self.reset.bind(reset)
        self.input.bind(channel)

    def receive(self):
        """Read and record one copied data value."""
        record("RECV", int(self.input.read()))

    def run(self):
        """Delay the first read, optionally delaying a second acknowledgement."""
        self.input.reset()
        sc.wait()
        if self.scenario == "stalled_consumer":
            sc.wait(3)
            self.receive()
        elif self.scenario == "delayed_ack":
            sc.wait(3)
            self.receive()
            sc.wait(3)
            self.receive()
        elif self.scenario == "reset_pending":
            sc.wait(8)
            self.receive()
        sc.sc_stop()


class ResetDriver(sc.sc_module):
    """Pulse the native synchronous active-low reset during pending traffic."""

    def __init__(self, reset):
        """Register one reset-driving thread for the reset scenario."""
        module_name = sc.sc_module_name("reset_driver")
        super().__init__(module_name)
        self.reset = sc.sc_out[bool]("reset")
        self.reset.bind(reset)
        self.sc_thread("run", self.run)

    def run(self):
        """Assert at 25 ns across clock edges, then release at 45 ns."""
        sc.wait(25, sc.SC_NS)
        self.reset.write(False)
        record("RESET_ASSERT")
        sc.wait(20, sc.SC_NS)
        self.reset.write(True)
        record("RESET_RELEASE")


def run(protocol, scenario):
    """Run one fresh protocol/scenario case and require a stopped simulation."""
    if protocol not in PROTOCOLS or scenario not in SCENARIOS:
        raise ValueError(f"unsupported handshake case: {protocol}/{scenario}")
    module = load_example(protocol)
    value_type = module.DATA
    clock = sc.sc_clock("clock", 10, sc.SC_NS, 0.5, 0, sc.SC_NS)
    channel = module.Channel("channel", value_type)
    reset = sc.sc_signal[bool]("reset")
    source = Source("source", protocol, scenario, value_type, module.Output)
    sink = Sink("sink", scenario, value_type, module.Input)
    source.bind(clock, reset, channel)
    sink.bind(clock, reset, channel)
    reset_driver = ResetDriver(reset) if scenario == "reset_pending" else None

    reset.write(False)
    sc.sc_start(1, sc.SC_NS)
    reset.write(True)
    sc.sc_start()
    assert sc.sc_end_of_simulation_invoked()
    assert source is not None and sink is not None
    if reset_driver is not None:
        assert reset_driver is not None


if __name__ == "__main__":
    run(*sys.argv[1:])
