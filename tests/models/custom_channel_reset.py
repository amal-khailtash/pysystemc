"""Verify B4 subclass and structural synchronization reset/error paths."""

from amal.eda.systemc import sc_core as sc

records = []


class Owner:
    """Suspend ordinary structural synchronization callbacks."""

    def lock(self):
        """Wait until reset/kill while preserving Python cleanup."""
        try:
            sc.wait(100, sc.SC_NS)
        finally:
            records.append(str(sc.sc_time_stamp()))
        return 0

    def trylock(self):
        """Reject lock without suspension."""
        return -1

    def unlock(self):
        """Release without waiting."""
        return 0


class Channel(sc.channel_interface(sc.sc_mutex_if)):
    """Implement a Python subclass through native virtual dispatch."""

    def __init__(self):
        """Construct native module before binding."""
        name = sc.sc_module_name("channel")
        super().__init__(name)

    def lock(self):
        """Suspend the subclass callback in a guarded wait."""
        return Owner.lock(self)

    def trylock(self):
        """Return a native contention result."""
        return -1

    def unlock(self):
        """Raise an ordinary Python callback exception."""
        raise ValueError("configured unlock failure")


name = sc.sc_module_name("top")
top = sc.sc_module(name)
channel = Channel()
ports = []
for owner in (Owner(), channel):
    port = sc.sc_port[sc.sc_mutex_if]()
    port(owner)
    ports.append(port)
del name
try:
    ports[1].get_interface().unlock()
except ValueError as error:
    assert "configured unlock" in str(error)
else:
    raise AssertionError("callback error lost")


def worker(port):
    """Invoke the native interface and require interrupted cleanup."""
    port.get_interface().lock()
    raise AssertionError("interrupted lock returned")


handles = [sc.sc_spawn(lambda port=port: worker(port)) for port in ports]


def control():
    """Reset at 2 ns and kill at 3 ns through native control boundaries."""
    sc.wait(2, sc.SC_NS)
    for handle in handles:
        handle.reset()
    sc.wait(1, sc.SC_NS)
    for handle in handles:
        handle.kill()


controller = sc.sc_spawn(control)
sc.sc_start(4, sc.SC_NS)
assert sorted(records) == ["2 ns", "2 ns", "3 ns", "3 ns"]
assert all(handle.terminated() for handle in (*handles, controller))
sc.sc_stop()
print("CUSTOM_CHANNEL_RESET_OK 2 4 ns")
