"""Python port of Accellera risc_cpu/bios.cpp and bios.h (Apache-2.0).

See upstream LICENSE/NOTICE. Native unsigned ports and guarded cycle waits
preserve the BIOS handshake; copied memory survives every native access.
"""

from pathlib import Path
from amal.eda.systemc import sc_core as sc


class BIOS(sc.sc_module):
    """Own BIOS memory and native clocked input/output ports."""

    def __init__(self, name):
        """Read BIOS image and register the source clocked process before binding."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        for port in ("datain", "addr"):
            setattr(self, port, sc.sc_in["unsigned"](port))
        for port in ("cs", "we"):
            setattr(self, port, sc.sc_in[bool](port))
        self.dataout = sc.sc_out["unsigned"]("dataout")
        self.bios_valid = sc.sc_out[bool]("bios_valid")
        self.stall_fetch = sc.sc_out[bool]("stall_fetch")
        self.CLK = sc.sc_in[bool]("CLK")
        values = [int(token, 16) for token in Path("bios.img").read_text().split()]
        self.memory = values + [0xFFFFFFFF] * (4000 - len(values))
        self.wait_cycles = 2
        print("** ALERT ** BIOS: initialize BIOS")
        self.sc_cthread("entry", self.run, self.CLK.pos())

    def init_param(self, delay):
        """Set the native number of memory access cycles before simulation."""
        self.wait_cycles = delay

    def run(self):
        """Preserve source BIOS reads, writes and valid-pulse cycle ordering."""
        while True:
            while True:
                sc.wait()
                if self.cs.read():
                    break
            address = self.addr.read()
            if address < 5:
                if self.we.read():
                    sc.wait(self.wait_cycles - 1)
                    self.memory[address] = self.datain.read()
                else:
                    if self.wait_cycles > 2:
                        sc.wait(self.wait_cycles - 2)
                    self.dataout.write(self.memory[address])
                    self.bios_valid.write(True)
                    sc.wait()
                    self.bios_valid.write(False)
                    sc.wait()
            else:
                self.bios_valid.write(False)
                sc.wait()
