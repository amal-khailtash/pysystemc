"""Port Accellera risc_cpu/fetch.cpp and fetch.h (Apache-2.0 LICENSE/NOTICE)."""

import ctypes
from amal.eda.systemc import sc_core as sc


class Fetch(sc.sc_module):
    """Own native fetch ports and source clocked instruction delivery state."""

    def __init__(self, name):
        """Register a guarded CTHREAD and retain ports until kernel shutdown."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        for port in ("ramdata", "branch_address", "int_vectno", "pred_branch_address"):
            setattr(self, port, sc.sc_in["unsigned"](port))
        for port in (
            "next_pc",
            "branch_valid",
            "stall_fetch",
            "interrupt",
            "bios_valid",
            "icache_valid",
            "pred_fetch",
            "pred_branch_valid",
        ):
            setattr(self, port, sc.sc_in[bool](port))
        for port in ("address", "smc_instruction", "instruction", "program_counter"):
            setattr(self, port, sc.sc_out["unsigned"](port))
        for port in (
            "ram_cs",
            "ram_we",
            "instruction_valid",
            "interrupt_ack",
            "branch_clear",
            "pred_fetch_valid",
            "reset",
        ):
            setattr(self, port, sc.sc_out[bool](port))
        self.CLK = sc.sc_in[bool]("CLK")
        self.memory_latency = 2
        self.sc_cthread("entry", self.run, self.CLK.pos())

    def init_param(self, delay):
        """Set source memory-latency cycles before simulation."""
        self.memory_latency = delay

    def wait_valid(self):
        """Always wait once before accepting BIOS or instruction-cache valid."""
        while True:
            sc.wait()
            if self.bios_valid.read() or self.icache_valid.read():
                return

    def emit(self, address, value, first=False):
        """Print source fetch value/address/time records and drive instruction ports."""
        ctypes.CDLL(None).fflush(None)
        border = "-----------------------" if first else "------------------------"
        print(f"{border}\nIFU : mem=0x{value:x}\nIFU : pc= {address:x} at CSIM {sc.sc_time_stamp()}\n{border}")
        self.instruction_valid.write(True)
        self.instruction.write(value)
        self.ram_cs.write(False)

    def run(self):
        """Preserve boot, interrupt, branch and sequential fetch cycle order."""
        address = 1
        lock = 0
        self.reset.write(True)
        self.ram_cs.write(True)
        self.ram_we.write(False)
        self.address.write(address)
        sc.wait(self.memory_latency)
        self.wait_valid()
        value = 0 if self.stall_fetch.read() else self.ramdata.read()
        self.emit(address, value, True)
        self.program_counter.write(address)
        sc.wait()
        self.instruction_valid.write(False)
        address += 1
        sc.wait()
        while True:
            if address == 5:
                self.reset.write(False)
            if self.interrupt.read():
                self.ram_cs.write(True)
                address = self.int_vectno.read()
                self.ram_we.write(False)
                sc.wait(self.memory_latency)
                value = self.ramdata.read()
                print("IF ALERT: **INTERRUPT**")
                self.emit(address, value)
                self.interrupt_ack.write(True)
                if self.next_pc.read():
                    address += 1
                sc.wait()
                self.instruction_valid.write(False)
                self.interrupt_ack.write(False)
                sc.wait()
            if self.branch_valid.read():
                print("IFU ALERT: **BRANCH**")
                lock += 1
                self.ram_cs.write(True)
                address = self.branch_address.read()
                self.ram_we.write(False)
                sc.wait(self.memory_latency)
                self.wait_valid()
                self.emit(address, self.ramdata.read())
                if self.next_pc.read():
                    address += 1
                sc.wait()
                self.instruction_valid.write(False)
                sc.wait()
            else:
                lock = 0
                self.ram_cs.write(True)
                self.address.write(address)
                self.ram_we.write(False)
                sc.wait(self.memory_latency)
                self.wait_valid()
                self.emit(address, self.ramdata.read())
                self.program_counter.write(address)
                self.branch_clear.write(False)
                if self.next_pc.read():
                    address += 1
                sc.wait()
                self.instruction_valid.write(False)
                sc.wait()
            if lock == 1:
                self.branch_clear.write(True)
                sc.wait()
