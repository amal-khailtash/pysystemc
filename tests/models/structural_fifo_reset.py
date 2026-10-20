"""Verify reset/kill unwind through structural FIFO native callback boundaries."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import sys
import cppyy

records = []
clock = sc.sc_clock("clock", sc.sc_time(100, sc.SC_NS), 0.5, sc.sc_time(50, sc.SC_NS), True)
first = sc.sc_event("first")
second = sc.sc_event("second")
wait_kinds = ("time", "event", "or", "and", "timed-event", "timed-or", "timed-and", "cycle", "count")
value_types = (
    int,
    "short",
    "unsigned short",
    "unsigned int",
    "long",
    "unsigned long",
    "long long",
    "unsigned long long",
    "float",
    "double",
    "long double",
    sc_dt.sc_bigint[128],
    sc_dt.sc_biguint[128],
    sc_dt.sc_biguint[137],
)
if len(sys.argv) == 3:
    value_types = value_types[int(sys.argv[1]) : int(sys.argv[2])]
paths_per_type = 18
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
        value_type = value_types[self.index // cases_per_type]
        if isinstance(value_type, str) or value_type is int:
            assert value == 17
        else:
            bit = 136 if value_type == sc_dt.sc_biguint[137] else 127
            assert value[bit] and value.to_uint64() == 17
            if value_type == sc_dt.sc_bigint[128]:
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


class CompoundOwner(FullOwner):
    """Supply FIFO debug callbacks while blocking get/peek wait in the owner."""

    def used(self):
        """Report empty FIFO storage."""
        return 0

    def size(self):
        """Report one available storage slot."""
        return 1

    def debug(self):
        """Inspect without changing storage or waiting."""

    def nb_peek(self, index=None):
        """Reject both ordinary and indexed reads of the empty FIFO."""
        return False, None

    def nb_poke(self, value, index):
        """Reject writes to nonexistent occupied entries."""
        return False


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
            sc.sc_fifo_blocking_in_if[value_type],
            tlm.tlm_blocking_get_if[value_type],
            tlm.tlm_blocking_peek_if[value_type],
            tlm.tlm_fifo_get_if[value_type],
            tlm.tlm_fifo_get_if[value_type],
            tlm.tlm_fifo_get_if[value_type],
            tlm.tlm_fifo_get_if[value_type],
        )
        * len(wait_kinds)
    )
for index, interface in enumerate(interfaces):
    port = sc.sc_port[interface]()
    path = index % paths_per_type
    if path >= 14:
        owner = CompoundOwner(index)
    elif path in (9, 10):
        owner = FullOwner(index)
    elif path in (7, 8):
        owner = PartialOwner(index, "get" if path == 7 else "peek")
    else:
        owner = Owner(index)
    port(owner)
    ports.append(port)
del name


output_helpers = {}
for index, interface in enumerate(interfaces):
    path = index % paths_per_type
    if path not in (11, 12, 13, 16, 17):
        continue
    operation = {11: "read", 12: "get", 13: "peek", 16: "get", 17: "peek"}[path]
    native_name = interface.__cpp_name__
    value_name = native_name.split("<", 1)[1].removesuffix(">")
    helper_name = f"reset_output_{index}"
    cppyy.cppdef(
        f"/** Borrow local output during a blocking callback; caller releases GIL, "
        f"adapter reacquires it; verify output before rethrowing scheduler unwind. */ "
        f"void {helper_name}({native_name}& channel) {{ "
        f"{value_name} output = 71; try {{ channel.{operation}(output); }} "
        f"catch (...) {{ if (output != static_cast<{value_name}>(71)) "
        "std::abort(); throw; } }"
    )
    helper = getattr(cppyy.gbl, helper_name)
    helper.__release_gil__ = True
    output_helpers[index] = helper


def worker(index):
    """Call a native interface and retain outer Python cleanup ordering."""
    records.append((index, "worker-start", str(sc.sc_time_stamp())))
    try:
        native = ports[index].get_interface()
        path = index % paths_per_type
        value_type = value_types[index // cases_per_type]
        wide = not (isinstance(value_type, str) or value_type is int)
        value = value_type(17) if wide else 17
        if wide:
            value[136 if value_type == sc_dt.sc_biguint[137] else 127] = True
        if index in output_helpers:
            output_helpers[index](native)
        elif path == 0:
            native.read()
        elif path in (1, 5, 7, 9, 14):
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
