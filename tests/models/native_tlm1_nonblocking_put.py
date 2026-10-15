"""Validate Python TLM-1 nonblocking put and stable readiness-event references."""

from amal.eda.systemc import sc_core as sc
from amal.eda.systemc import tlm


class Put(sc.channel_interface(tlm.tlm_nonblocking_put_if[int])):
    """Keep Python acceptance state while the native adapter owns the readiness event."""

    def __init__(self, name):
        """Initialize native identity and an initially unavailable channel."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.available = False
        self.values = []

    def put_value(self, value):
        """Accept one sample only when ready, without waiting or notifying."""
        if not self.available:
            return False
        self.values.append(value)
        self.available = False
        return True

    def can_put(self):
        """Report current readiness without side effects."""
        return self.available


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
channel = Put("put")
port = sc.sc_port[tlm.tlm_nonblocking_put_if[int]]("port")
port.bind(channel)
del module_name
records = []


def consumer():
    """Reject unavailable puts, wait for native event, then accept exactly one."""
    interface = port.get_interface()
    assert not interface.nb_can_put()
    assert not interface.nb_put(99)
    sc.wait(interface.ok_to_put())
    assert interface.nb_can_put()
    assert interface.nb_put(17)
    assert not interface.nb_put(23)
    records.append(str(sc.sc_time_stamp()))


def producer():
    """Publish readiness at2ns and notify through the channel-owned event."""
    sc.wait(2, sc.SC_NS)
    channel.available = True
    channel.notify_ready()


handles = [sc.sc_spawn(consumer, "consumer"), sc.sc_spawn(producer, "producer")]
sc.sc_start(3, sc.SC_NS)
assert channel.values == [17]
assert records == ["2 ns"], records
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("NATIVE_TLM1_NONBLOCKING_PUT_OK")
