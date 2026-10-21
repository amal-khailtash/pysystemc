"""Verify B3 adapter destruction follows reset/kill callback and worker cleanup."""

from amal.eda.systemc import sc_core as sc, tlm
import cppyy
import gc
import weakref

records = []


class Owner:
    """Suspend a structural blocking callback until scheduler unwind."""

    def __init__(self, label):
        """Retain a label for exact cleanup ordering."""
        self.label = label

    def read(self):
        """Wait with a finally block that runs before worker cleanup."""
        records.append((self.label, "callback", str(sc.sc_time_stamp())))
        try:
            sc.wait(100, sc.SC_NS)
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
    ("fifo", sc.sc_fifo_blocking_in_if[int], "read"),
    ("get", tlm.tlm_blocking_get_if[int], "get"),
    ("master", tlm.tlm_blocking_master_if[int, int], "get"),
    ("slave", tlm.tlm_blocking_slave_if[int, int], "peek"),
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


handles = [sc.sc_spawn(lambda index=index: worker(index), f"worker_{index}") for index in range(len(groups))]


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
