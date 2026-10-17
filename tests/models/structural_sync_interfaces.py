"""Validate structural mutex, semaphore, event queue and write/policy callbacks."""

from amal.eda.systemc import sc_core as sc, sc_dt


class Delegate:
    """Expose native channel methods as ordinary structural callbacks."""

    def __init__(self, channel):
        """Retain channel storage for native process lifetime."""
        self.channel = channel

    def __getattr__(self, name):
        """Forward required interface callbacks."""
        method = getattr(self.channel, name)
        if name in ("lock", "wait"):
            type(self.channel).__dict__[name].__release_gil__ = True
        return method


name = sc.sc_module_name("top")
top = sc.sc_module(name)
mutex = sc.sc_mutex("mutex")
sem = sc.sc_semaphore("sem", 0)
queue = sc.sc_event_queue("queue")
signal = sc.sc_signal[sc_dt.sc_biguint[137], sc.SC_MANY_WRITERS]("signal")
ports = []
for interface, channel in (
    (sc.sc_mutex_if, mutex),
    (sc.sc_semaphore_if, sem),
    (sc.sc_event_queue_if, queue),
    (sc.sc_signal_write_if[sc_dt.sc_biguint[137]], signal),
):
    port = sc.sc_port[interface]()
    port(Delegate(channel))
    ports.append(port)
del name
m, s, q, w = [port.get_interface() for port in ports]
assert s.trywait() == -1 and s.get_value() == 0
assert w.get_writer_policy() == sc.SC_MANY_WRITERS
value = sc_dt.sc_biguint[137](17)
value[136] = True
w.write(value)
value.__assign__(0)
seen = []


def worker():
    """Wait on structural mutex/semaphore calls with automatic GIL release."""
    sc.wait(sc.SC_ZERO_TIME)
    assert m.trylock() == -1
    assert m.lock() == 0
    seen.append(("mutex", sc.sc_time_stamp().to_double()))
    assert m.unlock() == 0
    assert s.wait() == 0
    seen.append(("semaphore", sc.sc_time_stamp().to_double()))


def controller():
    """Release resources and exercise both native notify overloads."""
    assert m.trylock() == 0
    sc.wait(2, sc.SC_NS)
    assert m.unlock() == 0
    sc.wait(1, sc.SC_NS)
    assert s.post() == 0


top.sc_thread("controller", controller)
top.sc_thread("worker", worker)
top.sc_method("queue_observer", lambda: seen.append(("queue", sc.sc_time_stamp().to_double())))
top.sensitive(q.default_event())
top.dont_initialize()
q.notify(9, sc.SC_NS)
q.cancel_all()
q.notify(sc.sc_time(1, sc.SC_NS))
q.notify(4, sc.SC_NS)
sc.sc_start(5, sc.SC_NS)
assert signal.read()[136] and signal.read().to_uint() == 17
assert seen == [("queue", 1000), ("mutex", 2000), ("semaphore", 3000), ("queue", 4000)], seen
assert s.get_value() == 0
sc.sc_stop()
print("SYNC_RESULT queue 1 mutex 2 semaphore 3 queue 4 value 17 high 1")
print("STRUCTURAL_SYNC_INTERFACES_OK")
