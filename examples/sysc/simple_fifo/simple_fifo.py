"""Port Accellera sysc/simple_fifo/simple_fifo.cpp (Apache-2.0).

See upstream LICENSE/NOTICE. The immediate-event ring buffer preserves native
availability markers; it intentionally differs from deferred native sc_fifo.
"""

from typing import Protocol
from amal.eda.systemc import sc_core as sc

MESSAGE = "Visit www.accellera.org and see what SystemC can do for you today!\n"


@sc.interface(methods={"write": ("void", "int value"), "reset": ("void", "")})
class WriteIf(Protocol):
    """Native structural interface for byte writes and channel reset."""

    def write(self, value: int) -> None:
        """Write one character code, waiting when the ring is full."""
        ...

    def reset(self) -> None:
        """Restore the empty ring before use."""
        ...


@sc.interface(methods={"read": ("int", ""), "num_available": ("int", "")})
class ReadIf(Protocol):
    """Native structural interface for reads and availability queries."""

    def read(self) -> int:
        """Return one character code, waiting when the ring is empty."""
        ...

    def num_available(self) -> int:
        """Return the current immediate ring occupancy."""
        ...


class FIFO:
    """Own ten ring slots and immediate native notification events."""

    def __init__(self):
        """Create retained storage and events before elaboration."""
        self.data = [0] * 10
        self.first = 0
        self.count = 0
        self.read_event = sc.sc_event("read_event")
        self.write_event = sc.sc_event("write_event")

    def write(self, value):
        """Wait on a full ring and copy the incoming byte immediately."""
        if self.count == 10:
            sc.wait(self.read_event)
        self.data[(self.first + self.count) % 10] = value
        self.count += 1
        self.write_event.notify()

    def read(self):
        """Wait on an empty ring and return the removed byte by value."""
        if self.count == 0:
            sc.wait(self.write_event)
        value = self.data[self.first]
        self.count -= 1
        self.first = (self.first + 1) % 10
        self.read_event.notify()
        return value

    def reset(self):
        """Clear ring state before any processes access it."""
        self.count = self.first = 0

    def num_available(self):
        """Return occupancy without advancing the kernel."""
        return self.count


class Top(sc.sc_module):
    """Retain channel owners, structural ports and both producer/consumer processes."""

    def __init__(self, name):
        """Bind native adapters and register producer before consumer."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.fifo = FIFO()
        self.output = sc.sc_port[WriteIf]("output")
        self.input = sc.sc_port[ReadIf]("input")
        self.output(self.fifo)
        self.input(self.fifo)
        self.received = []
        self.sc_thread("producer", self.produce)
        self.sc_thread("consumer", self.consume)

    def produce(self):
        """Send the upstream message and terminate without stopping simulation."""
        for character in MESSAGE:
            self.output.get_interface().write(ord(character))

    def consume(self):
        """Print characters and occupancy markers until native starvation."""
        print("\n")
        interface = self.input.get_interface()
        while True:
            character = chr(interface.read())
            self.received.append(character)
            print(character, end="", flush=True)
            count = interface.num_available()
            if count in (1, 9):
                print(f"<{count}>", end="", flush=True)


def run():
    """Run to starvation and verify all message bytes and final empty occupancy.

    Returns:
        None after native starvation at zero simulation time.
    """
    top = Top("Top1")
    sc.sc_start()
    assert "".join(top.received) == MESSAGE
    assert top.fifo.count == 0
    assert sc.sc_time_stamp() == sc.SC_ZERO_TIME


if __name__ == "__main__":
    run()
