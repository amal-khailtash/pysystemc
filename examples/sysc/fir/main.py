"""Port Accellera sysc/fir behavioral and RTL stimuli (Apache-2.0).

See upstream LICENSE/NOTICE. Native ports, fixed-width storage, delta updates
and guarded clocked waits preserve the source process boundaries and reset.
"""

from amal.eda.systemc import sc_core as sc, sc_dt as dt

COEFS = [-6, -4, 13, 16, -18, -41, 23, 154, 222, 154, 23, -41, -18, 16, 13, -4]


class Model(sc.sc_module):
    """Retain native signals and independent stimulus/filter/display processes."""

    def __init__(self, name, rtl=False):
        """Register either behavioral CTHREAD or RTL method processes."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_clock("clock")
        self.clock_port = sc.sc_in[bool]("clock_port")
        self.clock_port.bind(self.clock)
        self.reset = sc.sc_signal[bool]("reset")
        self.valid = sc.sc_signal[bool]("valid")
        self.sample = sc.sc_signal[int]("sample")
        self.ready = sc.sc_signal[bool]("ready")
        self.result = sc.sc_signal[int]("result")
        self.state_out = sc.sc_signal["unsigned"]("state_out")
        self.coefs = [dt.sc_int[9](value) for value in COEFS]
        self.shift = [dt.sc_int[8](0) for _ in range(16)]
        self.acc = dt.sc_int[19](0)
        self.cycle = 0
        self.send = dt.sc_int[8](0)
        self.records = []
        self.state = 0
        self.sc_method("stimulus", self.stimulus)
        self.sensitive(self.clock.posedge_event())
        self.dont_initialize()
        if rtl:
            self.sc_method("fsm", self.fsm)
            self.sensitive(self.clock.posedge_event())
            self.dont_initialize()
            self.sc_method("data", self.data)
            for signal in (self.reset, self.state_out, self.sample):
                self.sensitive(signal)
            self.dont_initialize()
        else:
            self.sc_cthread("filter", self.filter, self.clock_port.pos())
            self.reset_signal_is(self.reset, True)
        self.sc_method("display", self.display)
        self.sensitive(self.ready.posedge_event())
        self.dont_initialize()

    def stimulus(self):
        """Drive reset and one signed eight-bit sample every ten cycles."""
        self.cycle += 1
        self.reset.write(self.cycle < 4)
        self.valid.write(False)
        if self.cycle >= 4 and self.cycle % 10 == 0:
            self.valid.write(True)
            self.sample.write(int(self.send))
            print(f"Stimuli : {int(self.send)} at time {sc.sc_time_stamp().to_double():g}")
            self.send.__assign__(int(self.send) + 1)

    def filter(self):
        """Perform all sixteen native-width tap operations in one cycle."""
        for value in self.shift:
            value.__assign__(0)
        self.result.write(0)
        self.ready.write(False)
        sc.wait()
        while True:
            self.ready.write(False)
            while True:
                sc.wait()
                if self.valid.read():
                    break
            sample = dt.sc_int[8](self.sample.read())
            self.acc.__assign__(int(sample) * int(self.coefs[0]))
            for index in range(14, -1, -1):
                product = dt.sc_int[17](int(self.shift[index]) * int(self.coefs[index + 1]))
                self.acc.__assign__(int(self.acc) + int(product))
            self.shift_values(sample)
            self.result.write(int(self.acc))
            self.ready.write(True)
            sc.wait()

    def shift_values(self, sample):
        """Shift copied native samples in descending index order."""
        for index in range(14, -1, -1):
            self.shift[index + 1].__assign__(self.shift[index])
        self.shift[0].__assign__(sample)

    def fsm(self):
        """Update the RTL state output using preceding-cycle valid/reset."""
        if self.reset.read():
            self.state = 0
            return
        state = self.state
        self.state_out.write(state)
        if state == 0:
            self.state = 1
        elif state == 1:
            if self.valid.read():
                self.state = 2
        elif state in (2, 3):
            self.state += 1
        else:
            self.state = 1

    def data(self):
        """Accumulate RTL tap groups on reset/state/sample signal changes."""
        if self.reset.read():
            self.acc.__assign__(0)
            for value in self.shift:
                value.__assign__(0)
        self.result.write(0)
        self.ready.write(False)
        state = self.state_out.read()
        if state == 1:
            self.acc.__assign__(int(dt.sc_int[8](self.sample.read())) * int(self.coefs[0]))
        indices = {1: range(14, 10, -1), 2: range(10, 6, -1), 3: range(6, 2, -1), 4: range(2, -1, -1)}
        for index in indices.get(state, ()):
            self.acc.__assign__(int(self.acc) + int(self.shift[index]) * int(self.coefs[index + 1]))
        if state == 4:
            self.shift_values(self.sample.read())
            self.result.write(int(self.acc))
            self.ready.write(True)
        elif state not in indices:
            print("Information : Reset state")

    def display(self):
        """Capture output transitions and stop at twenty-four displayed items."""
        value = self.result.read()
        time = sc.sc_time_stamp().to_double()
        self.records.append((value, time))
        print(f"Display : {value}  at time {time:g}")
        if len(self.records) == 24:
            print(f"Simulation of 24 items finished at time {time:g}")
            sc.sc_stop()


def run(rtl=False):
    """Run one filter variant in a fresh kernel and verify completion.

    Args:
        rtl: Select the source RTL FSM/datapath rather than behavioral CTHREAD.
    Returns:
        None after twenty-four results and native terminal stop.
    """
    model = Model("process_body", rtl)
    sc.sc_start()
    assert len(model.records) == 24
    assert sc.sc_end_of_simulation_invoked()


if __name__ == "__main__":
    run()
