"""Port Accellera risc_cpu/floating.cpp (Apache-2.0 LICENSE/NOTICE).

Preserves the source's limited integer-significand model, including native
x86 shift-count masking. This model is not IEEE floating-point arithmetic.
"""

import ctypes
from amal.eda.systemc import sc_core as sc
from exec import signed32


class Floating(sc.sc_module):
    """Retain native floating-model ports and source cycle sequencing."""

    def __init__(self, name):
        """Create native ports and register the source CTHREAD."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.in_valid = sc.sc_in[bool]("in_valid")
        for port in ("opcode", "floata", "floatb"):
            setattr(self, port, sc.sc_in[int](port))
        self.dest = sc.sc_in["unsigned"]("dest")
        self.fdout = sc.sc_out[int]("fdout")
        self.fout_valid = sc.sc_out[bool]("fout_valid")
        self.fdestout = sc.sc_out["unsigned"]("fdestout")
        self.CLK = sc.sc_in[bool]("CLK")
        self.sc_cthread("entry", self.run, self.CLK.pos())

    def run(self):
        """Align, operate and reconstruct the source significand/exponent bits."""
        result = 0
        significand = 0
        label = ""
        prefix = "\t" * 7
        border = prefix + "-------------------------------"
        sc.wait(3)
        while True:
            while True:
                sc.wait()
                if self.in_valid.read():
                    break
            opcode, destination = self.opcode.read(), self.dest.read()
            a, b = self.floata.read() & 0xFFFFFFFF, self.floatb.read() & 0xFFFFFFFF
            sa, sb = a >> 31, b >> 31
            ea, eb = (a >> 23) & 255, (b >> 23) & 255
            ma, mb = a & 0x7FFFFF, b & 0x7FFFFF
            difference = ea - eb
            ctypes.CDLL(None).fflush(None)
            print(border)
            print(f"{prefix}FPU: A s=0x{sa:x}, exp=0x{ea:x}, mant=0x{ma:x}")
            print(f"{prefix}   : B s=0x{sb:x}, exp=0x{eb:x}, mant=0x{mb:x}")
            print(f"{prefix}   : exponent difference = {difference}\n{border}")
            if difference > 0:
                mb >>= difference & 31
                eb = ea
            else:
                ma >>= difference & 31
                ea = eb
            sc.wait()
            print(f"{border}\n{prefix}FPU: After Alignment")
            print(f"{prefix}FPU: A s=0x{sa:x}, exp=0x{ea:x}, mant=0x{ma:x}")
            print(f"{prefix}   : B s=0x{sb:x}, exp=0x{eb:x}, mant=0x{mb:x}\n{border}")
            if opcode == 0:
                label = "STALL"
            elif opcode == 3:
                label, significand = "FADD", (ma + mb) & 0xFFFFFFFF
            elif opcode == 4:
                label, significand = "FSUB", (ma - mb) & 0xFFFFFFFF
            elif opcode == 5:
                label, significand = "FMUL", (ma * mb) & 0xFFFFFFFF
                eb = (eb * 2) & 0xFFFFFFFF
            elif opcode == 6:
                label, significand = "FDIV", ma // mb
            else:
                print(f"{border}\n{prefix}FPU: Bad Opcode {opcode}.\n{border}")
            sc.wait()
            overflow = (significand & 0xFF800000) >> 23
            result = (significand << (overflow & 31)) & 0x7FFFFF
            exponent = (eb + overflow) & 0xFFFFFFFF
            result |= ((exponent << 23) & 0x7F800000) | (sa << 31)
            ctypes.CDLL(None).fflush(None)
            print(f"{border}\n{prefix}FPU: Result   Sign=0x{sa:x}.")
            print(f"{prefix}   : Result   Mantissa=0x{significand:x}.")
            print(f"{prefix}   : Result   Exponent=0x{exponent:x}.")
            print(f"{prefix}   : Overflow Mantissa=0x{overflow:x}.")
            print(f"{prefix}   : {label} FPU Output =0x{result:x}.")
            print(f"{prefix}   : at CSIM {sc.sc_time_stamp()}\n{border}")
            self.fdout.write(signed32(result))
            self.fout_valid.write(True)
            self.fdestout.write(destination)
            sc.wait()
            self.fout_valid.write(False)
            sc.wait()
