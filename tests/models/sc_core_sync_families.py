"""Compare public mutex/semaphore ownership, return values and blocking timing."""

from amal.eda.systemc import sc_core as sc
import cppyy

name = sc.sc_module_name("sync")
model = sc.sc_module(name)
mutex = sc.sc_mutex("mutex")
semaphore = sc.sc_semaphore("semaphore", 0)
mutex_if = cppyy.bind_object(cppyy.addressof(mutex), sc.sc_mutex_if)
sem_if = cppyy.bind_object(cppyy.addressof(semaphore), sc.sc_semaphore_if)
records = []
assert mutex.kind() == "sc_mutex" and semaphore.kind() == "sc_semaphore"
assert semaphore.get_value() == 0


def holder():
    """Repeated owner locking is idempotent and unlock wakes the contender."""
    assert mutex.lock() == 0 and mutex.trylock() == 0 and mutex.lock() == 0
    sc.wait(2, sc.SC_NS)
    assert mutex.unlock() == 0
    assert semaphore.post() == 0 and semaphore.get_value() == 1


def contender():
    """Verify non-owner failures, blocking acquisition and semaphore consumption."""
    sc.wait(1, sc.SC_NS)
    assert mutex_if.trylock() == -1 and mutex_if.unlock() == -1
    assert sem_if.trywait() == -1 and sem_if.get_value() == 0
    assert mutex_if.lock() == 0
    records.append(("mutex", sc.sc_time_stamp().value()))
    assert mutex_if.unlock() == 0
    assert sem_if.wait() == 0 and sem_if.get_value() == 0
    records.append(("semaphore", sc.sc_time_stamp().value()))
    assert sem_if.post() == 0 and sem_if.trywait() == 0
    assert sem_if.get_value() == 0
    sc.sc_stop()


model.sc_thread("holder", holder)
model.sc_thread("contender", contender)
del name
sc.sc_start()
assert records == [("mutex", 2000), ("semaphore", 2000)]
for kind, time in records:
    print("SYNC_FAMILY_REFERENCE", kind, time)
