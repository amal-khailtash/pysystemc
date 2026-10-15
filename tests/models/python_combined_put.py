"""Validate capacity-one blocking/nonblocking put through native combined ports."""

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm


class Queue:
    """Share capacity and ownership across blocking and immediate put callbacks."""

    def __init__(self):
        """Initialize a capacity-one queue and available-space event."""
        self.values = []
        self.calls = []
        self.space = sc.sc_event("space")

    def put(self, value):
        """Wait until space exists before storing a copied native value."""
        self.calls.append(("put", int(value)))
        while self.values:
            sc.wait(self.space)
        self.values.append(int(value))

    def nb_put(self, value):
        """Reject full capacity without waiting or altering stored samples."""
        self.calls.append(("nb_put", int(value)))
        if self.values:
            return False
        self.values.append(int(value))
        return True

    def nb_can_put(self):
        """Report capacity without modifying samples."""
        return not self.values


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
queue = Queue()
port = sc.sc_port[tlm.tlm_put_if[int]]("combined")
port.bind(queue)
del module_name
records = []
consumed = []


def producer():
    """Fill capacity, block for a second write, then await native readiness."""
    interface = port.get_interface()
    assert interface.nb_can_put()
    assert interface.nb_put(17)
    assert not interface.nb_can_put()
    assert not interface.nb_put(99)
    assert queue.values == [17]
    interface.put(23)
    records.append(str(sc.sc_time_stamp()))
    sc.wait(interface.ok_to_put())
    assert interface.nb_can_put()
    assert interface.nb_put(41)
    records.append(str(sc.sc_time_stamp()))


def consumer():
    """Free capacity at1ns and2ns, waking blocking/native event observers."""
    for expected in (17, 23):
        sc.wait(1, sc.SC_NS)
        assert queue.values == [expected]
        consumed.append(queue.values.pop(0))
        queue.space.notify(sc.SC_ZERO_TIME)
        port.get_interface().notify_ready()


handles = [sc.sc_spawn(producer, "producer"), sc.sc_spawn(consumer, "consumer")]
sc.sc_start(3, sc.SC_NS)
assert records == ["1 ns", "2 ns"]
assert consumed == [17, 23]
assert queue.values == [41]
assert queue.calls == [("nb_put", 17), ("nb_put", 99), ("put", 23), ("nb_put", 41)]
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("PYTHON_COMBINED_PUT_OK")
