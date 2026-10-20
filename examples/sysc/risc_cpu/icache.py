"""Port Accellera risc_cpu/icache.cpp and icache.h (Apache-2.0 LICENSE/NOTICE)."""

from pathlib import Path
from amal.eda.systemc import sc_core as sc


class ICache(sc.sc_module):
    """Own instruction memory and the source native cycle/valid protocol."""

    def __init__(self, name):
        """Load native unsigned instruction words and register one CTHREAD."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        for port in ("datain", "addr"):
            setattr(self, port, sc.sc_in["unsigned"](port))
        for port in ("cs", "we", "ld_valid"):
            setattr(self, port, sc.sc_in[bool](port))
        self.ld_data = sc.sc_in[int]("ld_data")
        self.dataout = sc.sc_out["unsigned"]("dataout")
        self.icache_valid = sc.sc_out[bool]("icache_valid")
        self.stall_fetch = sc.sc_out[bool]("stall_fetch")
        self.CLK = sc.sc_in[bool]("CLK")
        values = [int(token, 16) for token in Path("icache.img").read_text().split()]
        self.memory = values + [0xEEEEEEEE] * (500 - len(values))
        self.pid = 0
        self.wait_cycles = 2
        self.sc_cthread("entry", self.run, self.CLK.pos())

    def init_param(self, delay):
        """Retain the source parameter, unused by its actual read/write body."""
        self.wait_cycles = delay

    def run(self):
        """Preserve source process-id loading and one-cycle cache accesses."""
        while True:
            while True:
                sc.wait()
                if self.cs.read():
                    break
            address = self.addr.read()
            if address == 5:
                print("ICU ALERT: " + "*" * 69)
                print("         : " + "*" * 29 + "AFTER RESET" + "*" * 29)
                print("ICU ALERT: " + "*" * 69)
            if address < 5:
                continue
            if self.ld_valid.read():
                self.pid = self.ld_data.read()
                print(
                    f"------------------------\nICU: PID = [{self.pid}] at CSIM {sc.sc_time_stamp()}\n------------------------"
                )
                sc.wait()
                sc.wait()
            if self.we.read():
                sc.wait()
                if address < 500:
                    self.memory[address] = self.datain.read()
                else:
                    print("ICU ALERT: **MEMORY OUT OF RANGE**")
                sc.wait()
            else:
                sc.wait()
                self.dataout.write(self.memory[address] if address < 500 else 0xFFFFFFFF)
                if address >= 500:
                    print("ICU ALERT: **MEMORY OUT OF RANGE**")
                self.icache_valid.write(True)
                sc.wait()
                self.icache_valid.write(False)
                sc.wait()
