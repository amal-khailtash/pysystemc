"""Port Accellera simple_perf.cpp (Apache-2.0; upstream LICENSE/NOTICE).

The immediate-event FIFO retains source statistics and Linux rand traffic.
PYSYSTEMC_PERF_SEED selects a reproducible stream; absent it the source RNG
retains startup-dependent state.
Run each simulation in a fresh interpreter because SystemC owns singleton state.
"""

import ctypes
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "simple_fifo"))
from simple_fifo import MESSAGE, ReadIf, WriteIf
from amal.eda.systemc import sc_core as sc


class FIFO:
    """Own variable-size ring storage, events and source occupancy statistics."""

    def __init__(self, size):
        """Allocate size slots; callers must clamp size to 1..100000."""
        self.size = size
        self.data = [0] * size
        self.count = self.first = self.num_read = self.max_used = self.average = 0
        self.last_time = sc.SC_ZERO_TIME
        self.read_event = sc.sc_event("read_event")
        self.write_event = sc.sc_event("write_event")

    def write(self, value):
        """Wait on a full ring and notify readers after copying a byte."""
        if self.count == self.size:
            sc.wait(self.read_event)
        self.data[(self.first + self.count) % self.size] = value
        self.count += 1
        self.write_event.notify()

    def read(self):
        """Record entry time before waiting, then collect source fill statistics."""
        self.last_time = sc.sc_time_stamp()
        if self.count == 0:
            sc.wait(self.write_event)
        self.average += self.count
        self.max_used = max(self.max_used, self.count)
        self.num_read += 1
        value = self.data[self.first]
        self.count -= 1
        self.first = (self.first + 1) % self.size
        self.read_event.notify()
        return value

    def reset(self):
        """Clear immediate occupancy and ring position."""
        self.count = self.first = 0

    def num_available(self):
        """Return immediate occupancy without advancing simulation."""
        return self.count

    def report(self):
        """Print source destructor statistics after starvation."""
        print(f"\nFifo size is: {self.size}")
        print(f"Average fifo fill depth: {self.average / self.num_read:.6g}")
        print(f"Maximum fifo fill depth: {self.max_used}")
        print(f"Average transfer time per character: {self.last_time / self.num_read}")
        print(f"Total characters transferred: {self.num_read}")
        print(f"Total time: {self.last_time}")


class Top(sc.sc_module):
    """Retain native structural ports and producer/consumer callbacks."""

    def __init__(self, name, size):
        """Bind a retained FIFO and register producer before consumer."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.fifo = FIFO(size)
        self.output = sc.sc_port[WriteIf]("output")
        self.input = sc.sc_port[ReadIf]("input")
        self.output(self.fifo)
        self.input(self.fifo)
        self.sent = self.received = 0
        self.sc_thread("producer", self.produce)
        self.sc_thread("consumer", self.consume)

    def produce(self):
        """Send complete random bursts until at least 100000 bytes are sent."""
        libc = ctypes.CDLL(None)
        libc.rand.restype = ctypes.c_int
        if "PYSYSTEMC_PERF_SEED" in os.environ:
            libc.srand(int(os.environ["PYSYSTEMC_PERF_SEED"]))
        interface = self.output.get_interface()
        while True:
            count = 1 + int(19.0 * libc.rand() / 2147483647)
            for _ in range(count):
                interface.write(ord(MESSAGE[self.sent % len(MESSAGE)]))
                self.sent += 1
            if self.sent >= 100000:
                return
            sc.wait(1000, sc.SC_NS)

    def consume(self):
        """Check every byte while consuming at the source 100 ns interval."""
        interface = self.input.get_interface()
        while True:
            value = interface.read()
            assert value == ord(MESSAGE[self.received % len(MESSAGE)])
            self.received += 1
            sc.wait(100, sc.SC_NS)


def run(size=10):
    """Run the requested FIFO capacity, clamped like native atoi input.

    Args:
        size: Integer FIFO capacity; clamped to 1..100000.
    Returns:
        None after all bytes arrive and the kernel starves.
    """
    top = Top("Top1", min(100000, max(1, size)))
    sc.sc_start()
    assert top.sent == top.received == top.fifo.num_read >= 100000
    assert top.fifo.count == 0
    top.fifo.report()


if __name__ == "__main__":
    try:
        capacity = int(sys.argv[1]) if len(sys.argv) > 1 else 10
    except ValueError:
        capacity = 0
    run(capacity)
