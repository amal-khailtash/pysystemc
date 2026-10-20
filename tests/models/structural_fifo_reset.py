"""Verify reset/kill unwind through structural FIFO native callback boundaries."""

from amal.eda.systemc import sc_core as sc, tlm

records = []


class Owner:
    """Suspend native FIFO reads in a guarded Python wait."""

    def __init__(self, index):
        """Identify callback records across independent interfaces."""
        self.index = index

    def write(self, value):
        """Suspend a native FIFO output callback."""
        self.read()

    def put(self, value):
        """Suspend a native TLM put callback."""
        self.read()

    def peek(self):
        """Suspend a native TLM peek callback."""
        return self.read()

    def get(self):
        """Delegate TLM blocking-get to the guarded callback body."""
        return self.read()

    def read(self):
        """Record callback lifetime and propagate scheduler unwind through cleanup."""
        records.append((self.index, "callback-start", str(sc.sc_time_stamp())))
        try:
            sc.wait(100, sc.SC_NS)
            raise AssertionError("interrupted read returned")
        finally:
            records.append((self.index, "callback-cleanup", str(sc.sc_time_stamp())))


name = sc.sc_module_name("top")
top = sc.sc_module(name)
ports = []
interfaces = (
    sc.sc_fifo_blocking_in_if[int],
    tlm.tlm_blocking_get_if[int],
    sc.sc_fifo_blocking_out_if[int],
    tlm.tlm_blocking_put_if[int],
    tlm.tlm_blocking_peek_if[int],
)
for index, interface in enumerate(interfaces):
    port = sc.sc_port[interface]()
    port(Owner(index))
    ports.append(port)
del name


def worker(index):
    """Call a native interface and retain outer Python cleanup ordering."""
    records.append((index, "worker-start", str(sc.sc_time_stamp())))
    try:
        native = ports[index].get_interface()
        if index == 0:
            native.read()
        elif index == 1:
            native.get()
        elif index == 2:
            native.write(17)
        elif index == 3:
            native.put(17)
        else:
            native.peek()
        raise AssertionError("interrupted native read returned")
    finally:
        records.append((index, "worker-cleanup", str(sc.sc_time_stamp())))


handles = [sc.sc_spawn(lambda index=index: worker(index), f"worker_{index}") for index in range(len(interfaces))]


def controller():
    """Reset at two nanoseconds, then kill the restarted callback at three."""
    sc.wait(2, sc.SC_NS)
    for handle in handles:
        handle.reset()
    sc.wait(1, sc.SC_NS)
    for handle in handles:
        handle.kill()


control = sc.sc_spawn(controller, "controller")
sc.sc_start(4, sc.SC_NS)
for index in range(len(interfaces)):
    assert [record[1:] for record in records if len(record) == 3 and record[0] == index] == [
        ("worker-start", "0 s"),
        ("callback-start", "0 s"),
        ("callback-cleanup", "2 ns"),
        ("worker-cleanup", "2 ns"),
        ("worker-start", "2 ns"),
        ("callback-start", "2 ns"),
        ("callback-cleanup", "3 ns"),
        ("worker-cleanup", "3 ns"),
    ], records
assert all(handle.terminated() for handle in [*handles, control])
sc.sc_stop()
print("STRUCTURAL_FIFO_RESET_OK")
