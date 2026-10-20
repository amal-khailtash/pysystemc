"""Accellera mixed-endian interactive port (Apache-2.0 LICENSE/NOTICE).

Native word converters preserve bus-lane semantics. Buffers and payloads remain
retained through responses; each independent run requires a fresh interpreter.
"""

import sys
from array import array
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "common"))
from lt_models import Bus, LTTarget
from models import ns
from runtime import report
from amal.eda.systemc import sc_core as sc, tlm, tlm_utils


class Target(LTTarget):
    """Preserve source byte copies, including ignored byte enables after conversion."""

    def operation(self, gp):
        """Copy converted contiguous bytes with source memory bounds."""
        address, length = int(gp.get_address()), int(gp.get_data_length())
        assert address >= 0 and address + length <= 4096
        pointer = gp.get_data_ptr()
        write = gp.get_command() == tlm.TLM_WRITE_COMMAND
        for i in range(length):
            if write:
                self.memory[address + i] = pointer[i]
            else:
                pointer[i] = self.memory[address + i]
        gp.set_response_status(tlm.TLM_OK_RESPONSE)


class Top(sc.sc_module):
    """Retain both CPU sockets, buffers and ordered input ownership."""

    def __init__(self, name, commands):
        """Build source topology and retain native converter buffer owners."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.commands = commands
        self.bus = Bus("m_bus")
        self.targets = [Target("target_1", 201, 1), Target("target_2", 202, 4)]
        self.targets[0].accept_delay, self.targets[0].read_delay, self.targets[0].write_delay = 20, 100, 60
        self.sockets = [tlm_utils.callback_initiator_socket(f"cpu_{i}") for i in range(2)]
        for i in range(2):
            self.sockets[i].bind(self.bus.targets[i])
            self.bus.initiators[i].bind(self.targets[i].socket)
        self.buffers = [array("B", [0] * 1024) for _ in range(2)]
        self.payloads = [tlm.tlm_generic_payload() for _ in range(2)]
        for gp in self.payloads:
            gp.resize_extensions()
        self.results = []
        self.sc_thread("ui", self.run)

    def run(self):
        """Execute load/store/switch commands and preserve native result formatting."""
        active = [0, 1]
        for command in self.commands:
            command = command.rstrip("\n")
            if not command:
                continue
            if command[0] == "w":
                active.reverse()
                continue
            if command[0] == "q":
                active.pop(0)
                if not active:
                    break
                continue
            cpu = active[0]
            prompt = f"{101 + cpu} {'BE' if cpu == 0 else 'LE'}32 "
            if command[0] == "m":
                print("Memory map:\n  0 (16)\n  10000000 (16)")
                continue
            if command[0] in "hH?":
                print("enter one of the following commands (all numbers in hex):")
                print(
                    "  l8  addr nr_bytes          (load)\n  l16 addr nr_halfwords      (load)\n  l32 addr nr_words          (load)\n  s8  addr d0 d1 ...         (store)\n  s16 addr d0 d1 ...         (store)\n  s32 addr d0 d1 ...         (store)\n  m                          (memory map)\n  w                          (wait/switch initiator)\n  q                          (quit)"
                )
                continue
            if command[0] not in "ls":
                continue
            print(f"{prompt}:: cmd: {command}", flush=True)
            fields = command.split()
            bits = int(fields[0][1:])
            bits = bits if bits in (8, 16) else 32
            size = bits // 8
            address = int(fields[1], 16)
            write = command[0] == "s"
            values = [int(v, 16) for v in fields[2:]] if write else []
            length = len(values) if write else int(fields[2], 16)
            if length * size > 1024 or not length:
                continue
            buffer = self.buffers[cpu]
            if write:
                for index, value in enumerate(values):
                    for offset, byte in enumerate((value & ((1 << bits) - 1)).to_bytes(size, sys.byteorder)):
                        buffer[index * size + offset] = byte
            gp = self.payloads[cpu]
            gp.set_command(tlm.TLM_WRITE_COMMAND if write else tlm.TLM_READ_COMMAND)
            gp.set_address(address)
            gp.set_data_ptr(buffer)
            gp.set_data_length(length * size)
            gp.set_streaming_width(length * size)
            gp.set_byte_enable_ptr(0)
            gp.set_response_status(tlm.TLM_INCOMPLETE_RESPONSE)
            word = {8: "unsigned char", 16: "unsigned short", 32: "unsigned int"}[bits]
            convert = (cpu == 0) != (sys.byteorder == "big")
            if convert:
                tlm.tlm_to_hostendian_word[word](gp, 4)
            delay = ns(0)
            self.sockets[cpu].get_interface().b_transport(gp, delay)
            assert gp.get_response_status() == tlm.TLM_OK_RESPONSE
            sc.wait(delay)
            if convert:
                tlm.tlm_from_hostendian_word[word](gp, 4)
            print(
                f"{prompt}:: {'write' if write else 'read'} transaction length {length} (10) x {bits}-bit completed"
                + (", returning:" if not write else ""),
                flush=True,
            )
            if not write:
                values = [
                    int.from_bytes(bytes(buffer[i * size : (i + 1) * size]), sys.byteorder) for i in range(length)
                ]
                self.results.append((cpu, bits, address, values))
                print(prompt + "::" + "".join(f" {v:x}" for v in values), flush=True)
        for cpu in range(2):
            report(
                "me_traffic_generator.cpp",
                "me_traffic_generator_thread",
                f"Traffic Generator : {101 + cpu}\n=========================================================\n            ####  Traffic Generator Complete  #### ",
            )


def run(commands):
    """Run supplied stdin commands and retain native state through starvation.

    Args:
        commands: Iterable of source load/store/switch command lines.
    Returns:
        None after the finite command stream finishes.
    """
    top = Top("top", commands)
    sc.sc_start()
    assert top is not None


if __name__ == "__main__":
    run(sys.stdin.readlines())
