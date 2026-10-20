"""Port Accellera sysc/simple_bus test stimulus (Apache-2.0 LICENSE/NOTICE).

Python request storage is retained through native waits; native clock edges
preserve arbitration, slow-memory waits and blocking completion ordering.
"""

from dataclasses import dataclass, field
from amal.eda.systemc import sc_core as sc


@dataclass
class Request:
    """Retain a pending transfer's data and completion event until reuse."""

    priority: int
    data: list = field(default_factory=list)
    address: int = 0
    end: int = 0
    offset: int = 0
    write: bool = False
    status: str = "OK"
    done: object = field(default_factory=sc.sc_event)


class Bus(sc.sc_module):
    """Arbitrate the source's two unlocked masters and native slow-memory timing."""

    def __init__(self, clock):
        """Own memory and request storage and register both edge callbacks."""
        module_name = sc.sc_module_name("bus")
        super().__init__(module_name)
        self.clock = clock
        self.memory = [0] * 64
        self.requests = {}
        self.current = None
        self.wait_count = -1
        self.transfers = 0
        self.sc_method("main_action", self.action)
        self.sensitive(clock.negedge_event())
        self.dont_initialize()
        self.sc_method("slow_wait", self.slow_wait)
        self.sensitive(clock.posedge_event())
        self.dont_initialize()

    def slow_wait(self):
        """Advance the source slow-memory wait counter on each rising edge."""
        if self.wait_count >= 0:
            self.wait_count -= 1

    def submit(self, priority, data, address, write=False):
        """Retain a new unlocked request and return its native completion event."""
        request = self.requests.setdefault(priority, Request(priority))
        assert request.status == "OK"
        request.data = data
        request.address = address
        request.end = address + 4 * (len(data) - 1)
        request.offset = 0
        request.write = write
        request.status = "REQUEST"
        return request

    def burst(self, priority, data, address, write=False):
        """Wait for burst completion and then the following native positive edge."""
        request = self.submit(priority, data, address, write)
        sc.wait(request.done)
        sc.wait(self.clock.posedge_event())
        assert request.status == "OK"

    def action(self):
        """Perform one word or retain the selected request through slow waits."""
        if self.current is None:
            candidates = [r for r in self.requests.values() if r.status in ("REQUEST", "WAIT")]
            if not candidates:
                return
            self.current = min(candidates, key=lambda r: r.priority)
        request = self.current
        request.status = "WAIT"
        if request.address >= 128:
            if self.wait_count < 0:
                self.wait_count = 1
                return
            if self.wait_count != 0:
                return
        index = request.address // 4
        if request.write:
            self.memory[index] = request.data[request.offset]
        else:
            request.data[request.offset] = self.memory[index]
        self.transfers += 1
        request.address += 4
        request.offset += 1
        if request.address > request.end:
            request.status = "OK"
            request.done.notify()
        self.current = None


class Masters(sc.sc_module):
    """Register source blocking, polling and direct-monitor threads in order."""

    def __init__(self, bus, clock):
        """Retain bus and clock throughout the 10000 ns native run."""
        module_name = sc.sc_module_name("masters")
        super().__init__(module_name)
        self.bus, self.clock = bus, clock
        self.records = []
        for name, callback in (("blocking", self.blocking), ("nonblocking", self.nonblocking), ("direct", self.direct)):
            self.sc_thread(name, callback)
            self.sensitive(clock.posedge_event())

    def blocking(self):
        """Read/modify/write sixteen words then apply the source 300 ns pause."""
        while True:
            sc.wait()
            data = [0] * 16
            self.bus.burst(4, data, 0x4C)
            for index in range(16):
                data[index] += index
                sc.wait()
            self.bus.burst(4, data, 0x4C, True)
            sc.wait(300, sc.SC_NS)

    def nonblocking(self):
        """Poll each word request while preserving source address/count cycling."""
        address, count = 0x38, 0
        sc.wait()
        while True:
            data = [0]
            request = self.bus.submit(3, data, address)
            while request.status != "OK":
                sc.wait()
            data[0] += count
            count += 1
            request = self.bus.submit(3, data, address, True)
            while request.status != "OK":
                sc.wait()
            sc.wait(20, sc.SC_NS)
            sc.wait()
            address += 4
            if address > 0xB8:
                address, count = 0x38, 0

    def direct(self):
        """Print four monitored memory words every 100 ns without bus scheduling."""
        while True:
            values = self.bus.memory[30:34]
            line = f"{sc.sc_time_stamp()} top.master_d : mem[78:87] = ({', '.join(f'{x:x}' for x in values)})"
            self.records.append(line)
            print(line)
            sc.wait(100, sc.SC_NS)


def run():
    """Run the source fixed bus stimulus and assert traffic/time completion.

    Returns:
        None after 10000 ns and one hundred direct-monitor observations.
    """
    clock = sc.sc_clock("C1")
    bus = Bus(clock)
    masters = Masters(bus, clock)
    sc.sc_start(10000, sc.SC_NS)
    assert len(masters.records) == 100 and bus.transfers > 100
    assert sc.sc_time_stamp() == sc.sc_time(10000, sc.SC_NS)


if __name__ == "__main__":
    run()
