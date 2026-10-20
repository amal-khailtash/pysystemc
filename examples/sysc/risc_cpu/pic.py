"""Port Accellera risc_cpu/pic.cpp and pic.h (Apache-2.0 LICENSE/NOTICE)."""

from amal.eda.systemc import sc_core as sc


class PIC(sc.sc_module):
    """Retain native interrupt ports and source method sensitivity."""

    def __init__(self, name):
        """Register one noninitializing process sensitive to request changes."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        for port in ("ireq0", "ireq1", "ireq2", "ireq3", "cs", "rd_wr", "intack_cpu"):
            setattr(self, port, sc.sc_in[bool](port))
        for port in ("intreq", "intack"):
            setattr(self, port, sc.sc_out[bool](port))
        self.vectno = sc.sc_out["unsigned"]("vectno")
        self.sc_method("entry", self.run)
        for index in range(4):
            self.sensitive(getattr(self, f"ireq{index}"))
        self.dont_initialize()

    def run(self):
        """Preserve source request priority and acknowledge clearing.

        Source ireq3 maps to vector 2; this deliberate source match is retained.
        """
        for index in range(4):
            if getattr(self, f"ireq{index}").read():
                self.intreq.write(True)
                self.vectno.write(min(index, 2))
                break
        if self.intack_cpu.read() and self.cs.read():
            self.intreq.write(False)
