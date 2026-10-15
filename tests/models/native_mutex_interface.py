"""Implement the source-defined sc_mutex_if in a Python channel through ports."""

from amal.eda.systemc import sc_core as sc


class Mutex(sc.channel_interface(sc.sc_mutex_if)):
    """Track native process ownership and suspend contenders on a retained event."""

    def __init__(self, name):
        """Construct native combined base and retain ownership/event state."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.m_owner = None
        self.m_free = sc.sc_event("free")

    def lock(self):
        """Block until free and acquire ownership using the native process handle."""
        current = sc.sc_get_current_process_handle()
        while self.m_owner is not None and self.m_owner != current:
            sc.wait(self.m_free)
        self.m_owner = current
        return 0

    def trylock(self):
        """Return minus one when another process owns the mutex."""
        current = sc.sc_get_current_process_handle()
        if self.m_owner is not None and self.m_owner != current:
            return -1
        self.m_owner = current
        return 0

    def unlock(self):
        """Reject nonowners, otherwise notify blocked processes immediately."""
        if self.m_owner != sc.sc_get_current_process_handle():
            return -1
        self.m_owner = None
        self.m_free.notify()
        return 0


module_name = sc.sc_module_name("top")
model = sc.sc_module(module_name)
mutex_ = Mutex("mutex")
port = sc.sc_port[sc.sc_mutex_if]("port")
port.bind(mutex_)
del module_name
sc.sc_mutex_if.lock.__release_gil__ = True
records = []


def owner():
    """Hold the Python implementation through the native interface for two ns."""
    assert port.get_interface().lock() == 0
    records.append(("owner", str(sc.sc_time_stamp())))
    sc.wait(2, sc.SC_NS)
    assert port.get_interface().unlock() == 0


def contender():
    """Exercise nonowner errors and blocking native virtual dispatch."""
    sc.wait(1, sc.SC_NS)
    assert port.get_interface().unlock() == -1
    assert port.get_interface().trylock() == -1
    assert port.get_interface().lock() == 0
    records.append(("contender", str(sc.sc_time_stamp())))
    assert port.get_interface().unlock() == 0


handles = [sc.sc_spawn(owner, "owner"), sc.sc_spawn(contender, "contender")]
sc.sc_start(3, sc.SC_NS)
assert records == [("owner", "0 s"), ("contender", "2 ns")], records
assert all(handle.terminated() for handle in handles)
assert mutex_.m_owner is None
sc.sc_stop()
print("NATIVE_MUTEX_INTERFACE_OK")
