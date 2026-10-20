"""Port Accellera risc_cpu/dcache.cpp and dcache.h (Apache-2.0 LICENSE/NOTICE)."""

from pathlib import Path
from amal.eda.systemc import sc_core as sc


def signed(value):
    """Interpret an unsigned source memory word as a native signed int."""
    return value if value < 0x80000000 else value - 0x100000000


class DCache(sc.sc_module):
    """Own copied data/state/tag arrays and native directional ports."""

    def __init__(self, name):
        """Load image defaults and register native cycle-sensitive access."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.datain = sc.sc_in[int]("datain")
        for port in ("statein", "addr", "dest"):
            setattr(self, port, sc.sc_in["unsigned"](port))
        for port in ("cs", "we"):
            setattr(self, port, sc.sc_in[bool](port))
        self.dataout = sc.sc_out[int]("dataout")
        for port in ("destout", "stateout"):
            setattr(self, port, sc.sc_out["unsigned"](port))
        self.out_valid = sc.sc_out[bool]("out_valid")
        self.CLK = sc.sc_in[bool]("CLK")
        values = [int(token, 16) for token in Path("dcache.img").read_text().split()]
        self.memory = values + [0xDEADBEEF] * (4000 - len(values))
        self.states = [0] * 4000
        self.tags = list(range(len(values))) + [0xDEADBEEF] * (4000 - len(values))
        self.wait_cycles = 2
        print("** ALERT ** DCU: initialize Data Cache")
        self.sc_cthread("entry", self.run, self.CLK.pos())

    def init_param(self, delay):
        """Retain the source parameter, which its access process does not use."""
        self.wait_cycles = delay

    def run(self):
        """Preserve one-cycle reads/writes and source diagnostic records."""
        prefix = "\t" * 7
        border = prefix + "-------------------------------"
        while True:
            while True:
                sc.wait()
                if self.cs.read():
                    break
            destination = self.dest.read()
            address = self.addr.read()
            if self.we.read():
                sc.wait()
                self.out_valid.write(False)
                self.memory[address] = self.datain.read() & 0xFFFFFFFF
                self.states[address] = self.statein.read()
                self.tags[address] = self.addr.read()
                print(border)
                print(f"{prefix}DCU :St {self.memory[address]:x}->mem[{address}] at CSIM {sc.sc_time_stamp()}")
                print(border)
                sc.wait()
            else:
                sc.wait()
                self.dataout.write(signed(self.memory[address]))
                self.stateout.write(self.states[address])
                self.destout.write(destination)
                self.out_valid.write(True)
                print(border)
                print(f"{prefix}DCU :ld {self.memory[address]}<-mem[{address}]->R{destination}")
                state = {3: "M", 2: "E", 1: "S", 0: "I"}.get(self.states[address], "X")
                print(f"{prefix}DCU :Tag = 0x{self.tags[address]:x} ({state}) at CSIM {sc.sc_time_stamp()}")
                print(border)
                sc.wait()
                self.out_valid.write(False)
                sc.wait()
