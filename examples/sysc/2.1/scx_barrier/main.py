"""Port of SystemC 2.3.4 sysc/2.1/scx_barrier (Apache-2.0).

Adapted from Andy Goodrich's upstream example; see upstream LICENSE/NOTICE.
The barrier is implemented in Python using native SystemC events and waits.

Original source: examples/sysc/2.1/scx_barrier/main.cpp
"""

from amal.eda.systemc import main, sc_core as sc


class ScxBarrier:
    """One-shot barrier matching the upstream proposed scx_barrier class."""

    def __init__(self):
        self.remaining = 0
        self.event = sc.sc_event()

    def initialize(self, thread_n):
        """Set the number of participants before any process waits."""
        if thread_n <= 0:
            raise ValueError("barrier requires at least one participant")
        self.remaining = thread_n

    def wait(self):
        """Release all participants in the delta cycle of the final arrival."""
        if self.remaining <= 0:
            raise RuntimeError("one-shot barrier has already completed")
        self.remaining -= 1
        if self.remaining == 0:
            self.event.notify(sc.SC_ZERO_TIME)
        sc.wait(self.event)


class X(sc.sc_module):
    """Three native thread processes arriving at zero, five and eleven ns."""

    def __init__(self, name):
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.m_barrier = ScxBarrier()
        self.m_barrier.initialize(3)
        self.results = []
        self.sc_thread("a", self.a)
        self.sc_thread("b", self.b)
        self.sc_thread("c", self.c)

    def arrive(self, label):
        """Wait at the barrier and record the native release timestamp."""
        self.m_barrier.wait()
        timestamp = sc.sc_time_stamp().to_double()
        self.results.append((label, timestamp))
        print(f"{timestamp:f} - {label}")

    def a(self):
        """First delayed participant."""
        sc.wait(5.0, sc.SC_NS)
        self.arrive("a")

    def b(self):
        """Final participant triggers delta notification."""
        sc.wait(11.0, sc.SC_NS)
        self.arrive("b")

    def c(self):
        """Immediate participant waits for the other two."""
        self.arrive("c")


def sc_main(args):
    """Run the upstream clocked 1000-ns interval and verify all arrivals."""
    _clock = sc.sc_clock("clock")  # Retain the native clock through simulation.
    x = X("x")
    sc.sc_start(1000, sc.SC_NS)
    assert sorted(x.results) == [("a", 11000.0), ("b", 11000.0), ("c", 11000.0)]
    print("Program completed")
    return 0


if __name__ == "__main__":
    main()
