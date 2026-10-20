"""Port Accellera risc_cpu/mmxu.cpp (Apache-2.0 LICENSE/NOTICE)."""

import ctypes
from amal.eda.systemc import sc_core as sc
from exec import signed32


class MMX(sc.sc_module):
    """Native-port packed-byte arithmetic with source unsigned lane behavior."""

    def __init__(self, name):
        """Register source CTHREAD and retain native ports."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.mmx_valid = sc.sc_in[bool]("mmx_valid")
        for port in ("opcode", "mmxa", "mmxb"):
            setattr(self, port, sc.sc_in[int](port))
        self.dest = sc.sc_in["unsigned"]("dest")
        self.mmxdout = sc.sc_out[int]("mmxdout")
        self.mmxout_valid = sc.sc_out[bool]("mmxout_valid")
        self.mmxdestout = sc.sc_out["unsigned"]("mmxdestout")
        self.CLK = sc.sc_in[bool]("CLK")
        self.sc_cthread("entry", self.run, self.CLK.pos())

    def run(self):
        """Preserve all packed source operations and output cycle spacing."""
        result = 0
        sc.wait(3)
        while True:
            if not self.mmx_valid.read():
                sc.wait()
                continue
            a, b = self.mmxa.read() & 0xFFFFFFFF, self.mmxb.read() & 0xFFFFFFFF
            opcode, destination = self.opcode.read(), self.dest.read()
            aa, bb = [[(value >> (8 * i)) & 255 for i in range(4)] for value in (a, b)]
            label = {0: "STALL", 3: "PADD", 4: "PADDS", 5: "PSUB", 6: "PSUBS", 7: "PMADD", 8: "PACK", 9: "MMXCK"}.get(
                opcode, "INVALID"
            )
            if opcode in (3, 4, 5, 6, 9):
                lanes = []
                for x, y in zip(aa, bb):
                    value = x + y if opcode in (3, 4) else (x - y) & 0xFFFFFFFF
                    if opcode == 4:
                        value = min(value, 255)
                    elif opcode == 6:
                        value = max(x - y, 0)
                    elif opcode == 9:
                        value = 255 if x == y else 0
                    lanes.append(value)
                result = lanes[0]
                for i in range(1, 4):
                    result |= (lanes[i] << (8 * i)) & (255 << (8 * i))
            elif opcode == 7:
                lanes = [min(x * y, 255) for x, y in zip(aa, bb)]
                result = (min(lanes[3] + lanes[2], 255) << 16) | min(lanes[1] + lanes[0], 255)
            elif opcode == 8:
                result = (bb[2] << 24) | (bb[0] << 16) | (aa[2] << 8) | aa[0]
            elif opcode != 0:
                print(f"MMX:      Bad Opcode {opcode}.")
            result &= 0xFFFFFFFF
            sc.wait()
            self.mmxdout.write(signed32(result))
            self.mmxout_valid.write(True)
            self.mmxdestout.write(destination)
            ctypes.CDLL(None).fflush(None)
            prefix = "\t" * 7
            border = prefix + "-------------------------------"
            print(f"{border}\n{prefix}MMX : op= {label} A=0x {a:x} B=0x {b:x}")
            print(f"{prefix}MMX : C=0x {result:x}-> R{destination:x} at CSIM {sc.sc_time_stamp()}\n{border}")
            sc.wait()
            self.mmxout_valid.write(False)
            sc.wait()
