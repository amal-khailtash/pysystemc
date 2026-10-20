"""Port Accellera risc_cpu/paging.cpp and paging.h (Apache-2.0 LICENSE/NOTICE)."""

from amal.eda.systemc import sc_core as sc


class Paging(sc.sc_module):
    """Forward non-BIOS memory accesses with native clocked handshake timing."""

    def __init__(self, name):
        """Own native ports and register the guarded clocked process."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        for port in ("paging_din", "logical_address", "icache_din"):
            setattr(self, port, sc.sc_in["unsigned"](port))
        for port in ("paging_csin", "paging_wein", "icache_validin", "icache_stall"):
            setattr(self, port, sc.sc_in[bool](port))
        for port in ("paging_dout", "physical_address", "dataout"):
            setattr(self, port, sc.sc_out["unsigned"](port))
        for port in ("paging_csout", "paging_weout", "data_valid", "stall_ifu"):
            setattr(self, port, sc.sc_out[bool](port))
        self.CLK = sc.sc_in[bool]("CLK")
        self.sc_cthread("entry", self.run, self.CLK.pos())

    def run(self):
        """Preserve source chip-select, physical address and valid pulses."""
        while True:
            while True:
                sc.wait()
                if self.paging_csin.read():
                    break
            if self.logical_address.read() < 5:
                continue
            self.paging_csout.write(True)
            self.physical_address.write(self.logical_address.read())
            if self.paging_wein.read():
                self.paging_dout.write(self.paging_din.read())
                self.paging_weout.write(True)
                sc.wait()
                self.paging_csout.write(False)
                self.paging_weout.write(False)
            else:
                self.paging_weout.write(False)
                sc.wait()
                while True:
                    sc.wait()
                    if self.icache_validin.read():
                        break
                self.dataout.write(self.icache_din.read())
                self.data_valid.write(True)
                self.paging_csout.write(False)
                sc.wait()
                self.data_valid.write(False)
                sc.wait()
