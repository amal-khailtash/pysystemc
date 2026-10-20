"""Verify reset/kill unwind through structural FIFO native callback boundaries."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm

records = []
clock = sc.sc_clock("clock", sc.sc_time(100, sc.SC_NS), 0.5, sc.sc_time(50, sc.SC_NS), True)
first = sc.sc_event("first")
second = sc.sc_event("second")
wait_kinds = ("time", "event", "or", "and", "timed-event", "timed-or", "timed-and", "cycle", "count")
value_types = (
    int,
    "long long",
    "unsigned long long",
    "float",
    "double",
    "long double",
    sc_dt.sc_bigint[128],
    sc_dt.sc_biguint[128],
    sc_dt.sc_biguint[137],
)
paths_per_type = 11
cases_per_type = paths_per_type * len(wait_kinds)


class Owner:
    """Suspend native FIFO reads in a guarded Python wait."""

    def __init__(self, index):
        """Identify callback records across independent interfaces."""
        self.index = index

    def write(self, value):
        """Suspend a native FIFO output callback."""
        self.check_argument(value)
        self.read()

    def put(self, value):
        """Suspend a native TLM put callback."""
        self.check_argument(value)
        self.read()

    def check_argument(self, value):
        """Assert native copied writes retain arbitrary-width high bits."""
        type_index = self.index // cases_per_type
        if type_index < 6:
            assert value == 17
        else:
            bit = 136 if type_index == 8 else 127
            assert value[bit] and value.to_uint64() == 17
            if type_index == 6:
                assert value < 0

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
            kind = wait_kinds[(self.index // paths_per_type) % len(wait_kinds)]
            if kind == "time":
                sc.wait(100, sc.SC_NS)
            elif kind == "event":
                sc.wait(first)
            elif kind == "or":
                sc.wait(first | second)
            elif kind == "and":
                sc.wait(first & second)
            elif kind == "timed-event":
                sc.wait(100, sc.SC_NS, first)
            elif kind == "timed-or":
                sc.wait(100, sc.SC_NS, first | second)
            elif kind == "timed-and":
                sc.wait(100, sc.SC_NS, first & second)
            elif kind == "cycle":
                sc.wait()
            else:
                sc.wait(2)
            raise AssertionError("interrupted read returned")
        finally:
            records.append((self.index, "callback-cleanup", str(sc.sc_time_stamp())))


class FullOwner(Owner):
    """Expose both blocking and nonblocking get/peek callback families."""

    def nb_get(self):
        """Report no immediately available sample without waiting."""
        return False, None

    def nb_peek(self):
        """Report no immediately peekable sample without waiting."""
        return False, None

    def nb_can_get(self):
        """Report immediate get readiness."""
        return False

    def nb_can_peek(self):
        """Report immediate peek readiness."""
        return False


class PartialOwner:
    """Expose only one combined TLM operation to prevent full-interface selection."""

    def __init__(self, index, operation):
        """Retain guarded callback state and install the requested family."""
        self.owner = Owner(index)
        setattr(self, operation, getattr(self.owner, operation))
        setattr(self, "nb_" + operation, lambda: (False, None))
        setattr(self, "nb_can_" + operation, lambda: False)


name = sc.sc_module_name("top")
top = sc.sc_module(name)
ports = []
interfaces = []
for value_type in value_types:
    interfaces.extend(
        (
            sc.sc_fifo_blocking_in_if[value_type],
            tlm.tlm_blocking_get_if[value_type],
            sc.sc_fifo_blocking_out_if[value_type],
            tlm.tlm_blocking_put_if[value_type],
            tlm.tlm_blocking_peek_if[value_type],
            tlm.tlm_blocking_get_peek_if[value_type],
            tlm.tlm_blocking_get_peek_if[value_type],
            tlm.tlm_get_if[value_type],
            tlm.tlm_peek_if[value_type],
            tlm.tlm_get_peek_if[value_type],
            tlm.tlm_get_peek_if[value_type],
        )
        * len(wait_kinds)
    )
for index, interface in enumerate(interfaces):
    port = sc.sc_port[interface]()
    path = index % paths_per_type
    if path >= 9:
        owner = FullOwner(index)
    elif path >= 7:
        owner = PartialOwner(index, "get" if path == 7 else "peek")
    else:
        owner = Owner(index)
    port(owner)
    ports.append(port)
del name


def worker(index):
    """Call a native interface and retain outer Python cleanup ordering."""
    records.append((index, "worker-start", str(sc.sc_time_stamp())))
    try:
        native = ports[index].get_interface()
        path = index % paths_per_type
        type_index = index // cases_per_type
        wide = type_index >= 6
        value = value_types[type_index](17) if wide else 17
        if wide:
            value[136 if type_index == 8 else 127] = True
        if path == 0:
            native.read()
        elif path in (1, 5, 7, 9):
            native.get()
        elif path == 2:
            native.write(value)
        elif path == 3:
            native.put(value)
        else:
            native.peek()
        raise AssertionError("interrupted native read returned")
    finally:
        records.append((index, "worker-cleanup", str(sc.sc_time_stamp())))


handles = []
for index in range(len(interfaces)):
    options = sc.sc_spawn_options()
    options.set_sensitivity(clock.posedge_event())
    handles.append(sc.sc_spawn(lambda index=index: worker(index), f"worker_{index}", options))


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
