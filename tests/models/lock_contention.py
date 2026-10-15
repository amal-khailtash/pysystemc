"""Validate blocking native mutex and semaphore wakeup ordering."""

from amal.eda.systemc import sc_core as sc

sc.sc_mutex.lock.__release_gil__ = True
sc.sc_semaphore.wait.__release_gil__ = True
mutex = sc.sc_mutex("mutex")
semaphore = sc.sc_semaphore("semaphore", 0)
records = []


def owner():
    """Hold the mutex until 2 ns, then wake a blocked semaphore waiter."""
    guard = sc.sc_scoped_lock(mutex)
    records.append(("owner", str(sc.sc_time_stamp())))
    sc.wait(2, sc.SC_NS)
    assert guard.release()
    assert not guard.release()
    sc.wait(0.5, sc.SC_NS)
    assert semaphore.post() == 0


def waiter():
    """Block on the mutex, acquire at 2 ns and consume the semaphore token."""
    sc.wait(1, sc.SC_NS)
    assert mutex.trylock() == -1
    assert mutex.lock() == 0
    records.append(("mutex", str(sc.sc_time_stamp())))
    assert mutex.unlock() == 0
    assert semaphore.wait() == 0
    records.append(("semaphore", str(sc.sc_time_stamp())))


sc.sc_spawn(owner, "owner")
sc.sc_spawn(waiter, "waiter")
sc.sc_start(3, sc.SC_NS)
assert records == [("owner", "0 s"), ("mutex", "2 ns"), ("semaphore", "2500 ps")], records
assert semaphore.get_value() == 0
sc.sc_stop()
print("LOCK_CONTENTION_OK")
