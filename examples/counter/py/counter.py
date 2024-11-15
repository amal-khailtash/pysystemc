from amal.eda.systemc import sc_core, sc_dt, print_hierarchy


class Counter(sc_core.sc_module):

    def __init__(self, name: str):
        """Constructor for the counter
        Since this counter is a positive edge trigged one,
        We trigger the below block with respect to positive
        edge of the clock and also when ever reset changes state
        """
        module_name = sc_core.sc_module_name(name)
        super().__init__(module_name)

        print("Executing new")

        self.clock       = sc_core.sc_in_clk("clock")                       # Clock input of the design
        self.reset       = sc_core.sc_in[bool]("reset")                     # active high, synchronous Reset input
        self.enable      = sc_core.sc_in[bool]("enable")                    # Active high enable signal for counter
        self.counter_out = sc_core.sc_out[sc_dt.sc_uint[4]]("counter_out")  # 4 bit vector output of the counter

        # Local Variables
        self.count = sc_dt.sc_uint[4](0)

        # SC_NOBASE, SC_BIN, SC_OCT, SC_DEC, SC_HEX

        # print(dir(self.count))
        print(self.count.length())
        print(self.count.to_string(sc_dt.SC_BIN))
        print(self.count.to_string(sc_dt.SC_OCT))
        print(self.count.to_string(sc_dt.SC_DEC))
        print(self.count.to_string(sc_dt.SC_HEX))

        self.sc_method("incr_count", self.incr_count)
        self.sensitive(self.reset)
        self.sensitive(self.clock.pos())

    def incr_count(self):
        """Counter logic"""
        # At every rising edge of clock we check if reset is active
        if self.reset.read() == 1:
            # If active, we load the counter output with 4'b0000
            self.count = 0
            self.counter_out.write(self.count)
        # If enable is active, then we increment the counter
        elif self.enable.read() == 1:
            self.count = self.count + 1
            self.counter_out.write(self.count)
            print(f"@{sc_core.sc_time_stamp()} :: Incremented Counter {self.counter_out.read()}")

    def before_end_of_elaboration(self):
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Before End of Elaboration")
        print_hierarchy(True)

    def end_of_elaboration(self):
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Elaboration finished")

    def start_of_simulation(self):
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Simulation started")

    def end_of_simulation(self):
        print(f">>> {sc_core.sc_time_stamp()} - [ {self.name():12} ] - Simulation finished")
