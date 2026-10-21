"""Verify B4 subclass and structural synchronization reset/error paths."""

from amal.eda.systemc import sc_core as sc
import sys

semaphore = len(sys.argv) > 1
contract = sc.sc_semaphore_if if semaphore else sc.sc_mutex_if
blocking = "wait" if semaphore else "lock"
release = "post" if semaphore else "unlock"

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


Owner.wait = Owner.lock
Owner.trywait = Owner.trylock
Owner.post = Owner.unlock
Owner.get_value = lambda self: 0


class Channel(sc.channel_interface(contract)):
    """Implement a Python subclass through native virtual dispatch."""

    def __init__(self):
        """Construct native module before binding."""
        name = sc.sc_module_name("channel")
        super().__init__(name)

    def lock(self):
        """Suspend the subclass callback in a guarded wait."""
        return Owner.lock(self)

    def wait(self):
        """Suspend the semaphore callback through a guarded wait."""
        return Owner.lock(self)

    def trywait(self):
        """Reject a semaphore token without suspension."""
        return -1

    def post(self):
        """Raise a configured ordinary semaphore callback error."""
        raise ValueError("configured unlock failure")

    def get_value(self):
        """Return the native empty token count."""
        return 0

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
    port = sc.sc_port[contract]()
    port(owner)
    ports.append(port)
del name
try:
    getattr(ports[1].get_interface(), release)()
except ValueError as error:
    assert "configured unlock" in str(error)
else:
    raise AssertionError("callback error lost")


def worker(port):
    """Invoke the native interface and require interrupted cleanup."""
    getattr(port.get_interface(), blocking)()
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
