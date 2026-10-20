"""Port Accellera multicast packet-switch stimulus (Apache-2.0 LICENSE/NOTICE).

Native packet storage/ports and deferred signal updates preserve the upstream
ring-switch protocol. PYSYSTEMC_PACKET_SEED fixes the source's wall-time seed
for reproducible comparisons; absent that variable, wall time selects the seed.
"""

import ctypes
import os
import time
from pathlib import Path
from amal.eda.systemc import sc_core as sc
import cppyy

UPSTREAM = Path(__file__).resolve().parents[3] / "submodules/systemc/examples/sysc/pkt_switch"
cppyy.add_include_path(str(UPSTREAM))
cppyy.include("pkt.h")
PACKET = cppyy.gbl.pkt
LIBC = ctypes.CDLL(None)


class Sender(sc.sc_module):
    """Own one packet output and an upstream clocked random producer."""

    def __init__(self, name, signal, identity, clock):
        """Bind native packet/id/clock ports and retain their storage."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.output = sc.sc_out[PACKET]("output")
        self.output.bind(signal)
        self.identity = identity
        self.clock = sc.sc_in[bool]("clock")
        self.clock.bind(clock)
        self.sc_cthread("entry", self.run, self.clock.pos())

    def run(self):
        """Generate signed packet data and multicast masks using native rand."""
        LIBC.srand(int(os.environ.get("PYSYSTEMC_PACKET_SEED", int(time.time()))))
        sc.wait(8)
        while True:
            packet = PACKET()
            packet.data.__assign__(LIBC.rand() % 255)
            packet.id.__assign__(self.identity)
            dest = LIBC.rand() % 15 + 1
            for index in range(4):
                setattr(packet, f"dest{index}", bool(dest & (1 << index)))
            self.output.write(packet)
            print("..........................\nNew Packet Sent")
            print("Destination Addresses: " + "".join(f"{i + 1} " for i in range(4) if dest & (1 << i)))
            print(f"Packet Value: {int(packet.data)}\nSender ID: {self.identity + 1}\n..........................")
            sc.wait()
            sc.wait(1 + LIBC.rand() % 3)


class Receiver(sc.sc_module):
    """Observe native packet changes, ignoring the first like upstream."""

    def __init__(self, name, signal, identity):
        """Bind packet input and register a noninitializing method."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.input = sc.sc_in[PACKET]("input")
        self.input.bind(signal)
        self.identity = identity
        self.first = True
        self.count = 0
        self.sc_method("entry", self.run)
        self.sensitive(self.input)
        self.dont_initialize()

    def run(self):
        """Print received packet data/id in native receiver order."""
        if self.first:
            self.first = False
            return
        packet = self.input.read()
        self.count += 1
        for line in (
            "..........................",
            "New Packet Received",
            f"Receiver ID: {self.identity + 1}",
            f"Packet Value: {int(packet.data)}",
            f"Sender ID: {int(packet.id) + 1}",
            "..........................",
        ):
            print("                                  " + line)


class Switch(sc.sc_module):
    """Retain the four input/output queues and multicast register ring."""

    def __init__(self, inputs, outputs, control):
        """Bind native packet ports and static thread sensitivity."""
        module_name = sc.sc_module_name("SWITCH")
        super().__init__(module_name)
        self.inputs = [sc.sc_in[PACKET](f"in{i}") for i in range(4)]
        self.outputs = [sc.sc_out[PACKET](f"out{i}") for i in range(4)]
        self.control = sc.sc_in[bool]("control")
        self.control.bind(control)
        self.sc_thread("entry", self.run)
        for port, signal in zip(self.inputs, inputs):
            port.bind(signal)
            self.sensitive(port)
        for port, signal in zip(self.outputs, outputs):
            port.bind(signal)
        self.sensitive(self.control.pos())
        self.packets = self.dropped = 0

    def run(self):
        """Route each destination once, retaining native copies on all queue edges."""
        sc.wait()
        incoming = [[] for _ in range(4)]
        outgoing = [[] for _ in range(4)]
        ring = [None] * 4
        sc.wait()
        for _ in range(500):
            sc.wait()
            for index, port in enumerate(self.inputs):
                if port.event():
                    self.packets += 1
                    if len(incoming[index]) == 4:
                        self.dropped += 1
                    else:
                        incoming[index].append(PACKET(port.read()))
            for index in range(4):
                if incoming[index] and ring[index] is None:
                    ring[index] = incoming[index].pop(0)
            if self.control.read() and self.control.event():
                ring = ring[1:] + ring[:1]
                for index, packet in enumerate(ring):
                    if packet is not None and getattr(packet, f"dest{index}") and len(outgoing[index]) < 4:
                        outgoing[index].append(PACKET(packet))
                        setattr(packet, f"dest{index}", False)
                        if not any(getattr(packet, f"dest{i}") for i in range(4)):
                            ring[index] = None
                for index, queue in enumerate(outgoing):
                    if queue:
                        self.outputs[index].write(queue.pop(0))
        sc.sc_stop()
        print("End of switch operation...")
        print(f"Total number of packets received: {self.packets}")
        print(f"Total number of packets dropped: {self.dropped}")
        print(f"Percentage packets dropped:  {self.dropped * 100 // self.packets}")


class Control(sc.sc_module):
    """Toggle switch control on every second source clock edge."""

    def __init__(self, clock, signal):
        """Register the upstream initializing clock-sensitive method."""
        module_name = sc.sc_module_name("SWITCH_CLK")
        super().__init__(module_name)
        self.clock = sc.sc_in[bool]("clock")
        self.clock.bind(clock)
        self.output = sc.sc_out[bool]("output")
        self.output.bind(signal)
        self.value = False
        self.sc_method("entry", self.run)
        self.sensitive(self.clock.pos())
        self.dont_initialize()

    def run(self):
        """Write the old toggle before flipping local state."""
        self.output.write(self.value)
        self.value = not self.value


def run():
    """Run the fixed 500-activation upstream scenario and verify traffic.

    Returns:
        None after switch stop and nonempty receiver activity.
    """
    inputs = [sc.sc_signal[PACKET](f"in{i}") for i in range(4)]
    outputs = [sc.sc_signal[PACKET](f"out{i}") for i in range(4)]
    control = sc.sc_signal[bool]("control")
    clock1 = sc.sc_clock("CLOCK1", 75, sc.SC_NS, 0.5, 0, sc.SC_NS)
    clock2 = sc.sc_clock("CLOCK2", 30, sc.SC_NS, 0.5, 10, sc.SC_NS)
    senders = [Sender(f"SENDER{i}", inputs[i], i, clock1) for i in range(4)]
    toggle = Control(clock2, control)
    switch = Switch(inputs, outputs, control)
    receivers = [Receiver(f"RECEIVER{i}", outputs[i], i) for i in range(4)]
    sc.sc_start(0, sc.SC_NS)
    sc.sc_start()
    assert switch.packets > 0 and sum(receiver.count for receiver in receivers) > 0
    assert sc.sc_end_of_simulation_invoked()
    assert len(senders) == 4 and toggle is not None


if __name__ == "__main__":
    run()
