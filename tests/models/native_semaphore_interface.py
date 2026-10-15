"""Validate a Python implementation of source-native semaphore interface methods."""

from amal.eda.systemc import sc_core as sc


class Semaphore(sc.channel_interface(sc.sc_semaphore_if)):
    """Retain token count and wake native processes through a SystemC event."""

    def __init__(self, name, initial):
        """Initialize native channel identity and Python semaphore state."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.m_value = initial
        self.m_free = sc.sc_event("free")

    def wait(self):
        """Suspend until a token is available, then consume exactly one."""
        while self.m_value <= 0:
            sc.wait(self.m_free)
        self.m_value -= 1
        return 0

    def trywait(self):
        """Consume a token immediately or return minus one without waiting."""
        if self.m_value <= 0:
            return -1
        self.m_value -= 1
        return 0

    def post(self):
        """Add a token and notify waiting processes immediately."""
        self.m_value += 1
        self.m_free.notify()
        return 0

    def get_value(self):
        """Return the current number of available tokens."""
        return self.m_value


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
semaphore = Semaphore("semaphore", 0)
port = sc.sc_port[sc.sc_semaphore_if]("port")
port.bind(semaphore)
del module_name
sc.sc_semaphore_if.wait.__release_gil__ = True
records = []


def consumer():
    """Reject an empty nonblocking read and block until the producer posts."""
    interface = port.get_interface()
    assert interface.trywait() == -1
    assert interface.wait() == 0
    records.append(str(sc.sc_time_stamp()))
    assert interface.get_value() == 0


def producer():
    """Post one token at2ns and verify a subsequent nonblocking consumption."""
    sc.wait(2, sc.SC_NS)
    interface = port.get_interface()
    assert interface.post() == 0
    sc.wait(sc.SC_ZERO_TIME)
    assert interface.post() == 0
    assert interface.trywait() == 0
    assert interface.trywait() == -1


handles = [sc.sc_spawn(consumer, "consumer"), sc.sc_spawn(producer, "producer")]
sc.sc_start(3, sc.SC_NS)
assert records == ["2 ns"], records
assert semaphore.m_value == 0
assert all(handle.terminated() for handle in handles)
sc.sc_stop()
print("NATIVE_SEMAPHORE_INTERFACE_OK")
