"""Port Accellera SystemC sysc/2.1/dpipe (Apache-2.0; upstream LICENSE/NOTICE).

A four-stage 121-bit pipeline preserves native export and deferred signal updates.

Original source: examples/sysc/2.1/dpipe/main.cpp
"""

from amal.eda.systemc import sc_core as sc, sc_dt as dt

ATOM = dt.sc_biguint[121]


class DelayPipe(sc.sc_module):
    """Clock an arbitrary native datatype through a configurable stage count."""

    def __init__(self, name, value_type, stages):
        """Own channels/exports and register native edge sensitivity."""
        if stages < 2:
            raise ValueError("delay pipe requires at least two stages")
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_in_clk("clock")
        self.pipe = [sc.sc_signal[value_type](f"stage_{i}") for i in range(stages)]
        self.input = sc.sc_export[sc.sc_signal_inout_if[value_type]]("input")
        self.output = sc.sc_export[sc.sc_signal_in_if[value_type]]("output")
        self.input.bind(self.pipe[0])
        self.output.bind(self.pipe[-1])
        self.sc_method("rachet", self.rachet)
        self.sensitive(self.clock.pos())

    def rachet(self):
        """Schedule stage writes using values from the preceding update phase."""
        for index in range(len(self.pipe) - 1, 0, -1):
            self.pipe[index].write(self.pipe[index - 1].read())


class Reader(sc.sc_module):
    """Observe pipeline output before each edge's deferred update."""

    def __init__(self, name):
        """Register a noninitializing edge observer with retained ports."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_in_clk("clock")
        self.input = sc.sc_in[ATOM]("input")
        self.records = []
        self.sc_method("extract", self.extract)
        self.sensitive(self.clock.pos())
        self.dont_initialize()

    def extract(self):
        """Print and retain native time/value observations."""
        record = (int(sc.sc_time_stamp().to_double()), self.input.read().to_uint64())
        self.records.append(record)
        print(f"{record[0]}: {record[1]}")


class Writer(sc.sc_module):
    """Write a native-width incrementing value at each rising edge."""

    def __init__(self, name):
        """Register an initializing edge writer with native ports/storage."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_in_clk("clock")
        self.output = sc.sc_inout[ATOM]("output")
        self.counter = ATOM(0)
        self.sc_method("insert", self.insert)
        self.sensitive(self.clock.pos())

    def insert(self):
        """Write the old counter value and increment within native width."""
        self.output.write(self.counter)
        self.counter.__assign__(ATOM(self.counter.to_uint64() + 1))


clock = sc.sc_clock("clock")
pipe = DelayPipe("pipe", ATOM, 4)
reader = Reader("reader")
writer = Writer("writer")
for model in (pipe, reader, writer):
    model.clock.bind(clock)
reader.input.bind(pipe.output.get_interface())
writer.output.bind(pipe.input.get_interface())
sc.sc_start(10, sc.SC_NS)
assert len(reader.records) == 10, reader.records
assert [value for _, value in reader.records] == [0, 0, 0, 0, 1, 2, 3, 4, 5, 6], reader.records
sc.sc_stop()
