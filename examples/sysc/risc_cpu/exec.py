"""Port Accellera risc_cpu/exec.cpp and exec.h (Apache-2.0 LICENSE/NOTICE)."""

import ctypes
from amal.eda.systemc import sc_core as sc


def signed32(value):
    """Interpret the low native 32-bit integer bits as signed."""
    value &= 0xFFFFFFFF
    return value if value < 0x80000000 else value - 0x100000000


class Execute(sc.sc_module):
    """Implement source integer ALU timing and output flags using native ports."""

    def __init__(self, name):
        """Register one native clocked process and retain all source ports."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        for port in ("opcode", "add1", "dina", "dinb"):
            setattr(self, port, sc.sc_in[int](port))
        for port in ("reset", "in_valid", "negate", "shift_sel", "forward_A", "forward_B"):
            setattr(self, port, sc.sc_in[bool](port))
        self.dest = sc.sc_in["unsigned"]("dest")
        for port in ("C", "V", "Z", "out_valid"):
            setattr(self, port, sc.sc_out[bool](port))
        self.dout = sc.sc_out[int]("dout")
        self.destout = sc.sc_out["unsigned"]("destout")
        self.CLK = sc.sc_in[bool]("CLK")
        self.sc_cthread("entry", self.run, self.CLK.pos())

    def run(self):
        """Preserve one/two-cycle ALU work and source signed int assignments."""
        result = 0
        sc.wait(3)
        while True:
            if not self.in_valid.read():
                sc.wait()
                continue
            a, b = self.dina.read(), self.dinb.read()
            opcode, destination = self.opcode.read(), self.dest.read()
            if opcode in (1, 3):
                result = signed32(a + b)
            elif opcode in (2, 4):
                result = signed32(a - b)
            elif opcode == 5:
                result = signed32(a * b)
            elif opcode == 6:
                if b:
                    result = abs(a) // abs(b) * (-1 if (a < 0) != (b < 0) else 1)
                else:
                    print("Division Exception - Divide by zero ")
            elif opcode == 7:
                result = signed32(~(a & b))
            elif opcode == 8:
                result = signed32(a & b)
            elif opcode == 9:
                result = signed32(a | b)
            elif opcode == 10:
                result = signed32(a ^ b)
            elif opcode == 11:
                result = signed32(~a)
            elif opcode == 12:
                result = signed32(a << (b & 31))
            elif opcode == 13:
                result = a >> (b & 31)
            elif opcode == 14:
                quotient = abs(a) // abs(b) * (-1 if (a < 0) != (b < 0) else 1)
                result = a - quotient * b
            if 0 <= opcode <= 14:
                sc.wait()
                if opcode in (5, 6):
                    sc.wait()
            else:
                print(f"ALU:      Bad Opcode {opcode}.")
            self.dout.write(signed32(result))
            self.out_valid.write(True)
            self.destout.write(destination)
            self.Z.write(result == 0)
            self.C.write(bool(abs(result) & (1 << 32)))
            self.V.write(abs(result) > (1 << 32))
            ctypes.CDLL(None).fflush(None)
            prefix = "\t" * 7
            border = prefix + "-------------------------------"
            print(f"{border}\n{prefix}ALU : op= {opcode} A= {a} B= {b}")
            print(f"{prefix}ALU : R= {result}-> R{destination} at CSIM {sc.sc_time_stamp()}\n{border}")
            sc.wait()
            self.out_valid.write(False)
            sc.wait()
