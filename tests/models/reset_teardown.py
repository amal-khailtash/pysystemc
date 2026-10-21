"""Verify B3 adapter destruction follows reset/kill callback and worker cleanup."""

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy
import gc
import weakref
import sys

records = []
wait_kind = sys.argv[1] if len(sys.argv) > 1 else "time"
type_name = sys.argv[2] if len(sys.argv) > 2 else "int"
datatype = {
    "int": "int",
    "long_long": "long long",
    "wide137": sc_dt.sc_biguint[137],
    "signed128": sc_dt.sc_bigint[128],
}[type_name]
first = sc.sc_event("first")
second = sc.sc_event("second")
clock = sc.sc_clock("clock", sc.sc_time(100, sc.SC_NS), 0.5, sc.sc_time(50, sc.SC_NS), True)


def suspend():
    """Suspend through a guarded wait family until reset or kill."""
    if wait_kind == "time":
        sc.wait(100, sc.SC_NS)
    elif wait_kind == "event":
        sc.wait(first)
    elif wait_kind == "or":
        sc.wait(first | second)
    elif wait_kind == "and":
        sc.wait(first & second)
    elif wait_kind == "timed-event":
        sc.wait(100, sc.SC_NS, first)
    elif wait_kind == "timed-or":
        sc.wait(100, sc.SC_NS, first | second)
    elif wait_kind == "timed-and":
        sc.wait(100, sc.SC_NS, first & second)
    elif wait_kind == "cycle":
        sc.wait()
    elif wait_kind == "count":
        sc.wait(2)
    else:
        raise ValueError(wait_kind)


class Owner:
    """Suspend a structural blocking callback until scheduler unwind."""

    def __init__(self, label):
        """Retain a label for exact cleanup ordering."""
        self.label = label

    def read(self):
        """Wait with a finally block that runs before worker cleanup."""
        records.append((self.label, "callback", str(sc.sc_time_stamp())))
        try:
            suspend()
            raise AssertionError("interrupted callback returned")
        finally:
            records.append((self.label, "callback-cleanup", str(sc.sc_time_stamp())))

    def get(self):
        """Suspend a TLM output callback."""
        return self.read()

    def peek(self):
        """Suspend a TLM peek callback."""
        return self.read()

    def put(self, value):
        """Suspend a copied TLM input callback."""
        assert value == 17
        self.read()

    def transport(self, value):
        """Suspend a TLM1 transport callback."""
        assert value == 17
        return self.read()

    def b_transport(self, payload, delay):
        """Suspend a borrowed TLM2 blocking payload callback."""
        self.read()


name = sc.sc_module_name("top")
top = sc.sc_module(name)
groups = []
for label, interface, method in (
    ("fifo", sc.sc_fifo_blocking_in_if[datatype], "read"),
    ("get", tlm.tlm_blocking_get_if[datatype], "get"),
    ("master", tlm.tlm_blocking_master_if[datatype, datatype], "get"),
    ("slave", tlm.tlm_blocking_slave_if[datatype, datatype], "peek"),
    ("transport", tlm.tlm_transport_if[int, int], "transport"),
    ("blocking", tlm.tlm_blocking_transport_if[tlm.tlm_generic_payload], "b_transport"),
):
    owner = Owner(label)
    port = sc.sc_port[interface]()
    port(owner)
    groups.append((label, port, method, weakref.ref(owner), port.get_interface().name()))
del owner, port, name


def worker(index):
    """Invoke the borrowed interface and record unwind cleanup."""
    label, port, method, reference, adapter_name = groups[index]
    records.append((label, "worker", str(sc.sc_time_stamp())))
    try:
        callback = getattr(port.get_interface(), method)
        if method == "transport":
            callback(17)
        elif method == "b_transport":
            callback(tlm.tlm_generic_payload(), sc.sc_time(0, sc.SC_NS))
        else:
            callback()
        raise AssertionError("interrupted worker returned")
    finally:
        records.append((label, "worker-cleanup", str(sc.sc_time_stamp())))


handles = []
for index in range(len(groups)):
    options = sc.sc_spawn_options()
    options.set_sensitivity(clock.posedge_event())
    handles.append(sc.sc_spawn(lambda index=index: worker(index), f"worker_{index}", options))


def controller():
    """Reset callbacks at 2 ns and kill the restarted callbacks at 3 ns."""
    sc.wait(2, sc.SC_NS)
    for handle in handles:
        handle.reset()
    sc.wait(1, sc.SC_NS)
    for handle in handles:
        handle.kill()


control = sc.sc_spawn(controller, "controller")
sc.sc_start(4, sc.SC_NS)
assert all(handle.terminated() for handle in (*handles, control))
for label, port, method, reference, adapter_name in groups:
    assert [entry[1:] for entry in records if entry[0] == label] == [
        ("worker", "0 s"),
        ("callback", "0 s"),
        ("callback-cleanup", "2 ns"),
        ("worker-cleanup", "2 ns"),
        ("worker", "2 ns"),
        ("callback", "2 ns"),
        ("callback-cleanup", "3 ns"),
        ("worker-cleanup", "3 ns"),
    ], records
sc.sc_stop()
assert all(reference() is not None for label, port, method, reference, adapter_name in groups)
for label, port, method, reference, adapter_name in groups:
    port.__destruct__()
assert sc.teardown_interfaces() == 6
records.append(("all", "deleted", str(sc.sc_time_stamp())))
gc.collect()
assert all(reference() is None for label, port, method, reference, adapter_name in groups)
assert all(
    not cppyy.gbl.sc_core.sc_find_object(adapter_name) for label, port, method, reference, adapter_name in groups
)
assert records[-1] == ("all", "deleted", "4 ns")
assert sc.teardown_interfaces() == 0
print("RESET_TEARDOWN_OK 6 4 ns")
for entry in records:
    if entry[0] in ("fifo", "get", "slave", "all"):
        print("TEARDOWN_REFERENCE", *entry)
