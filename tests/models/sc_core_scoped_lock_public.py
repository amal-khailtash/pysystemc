"""Verify guarded scoped lock acquisition, native release and unwind cleanup."""

import gc
from amal.eda.systemc import sc_core as sc

import cppyy

cppyy.cppdef("""
/** Expose protected lock state to verify destruction from a process. */
struct ScopedMutexProbe : sc_core::sc_mutex {
  explicit ScopedMutexProbe(const char* name) : sc_mutex(name) {}
  bool active() const { return in_use(); }
};
""")
mutex = cppyy.gbl.ScopedMutexProbe("mutex")
records = []
try:
    sc.sc_scoped_lock(object())
except TypeError:
    pass
else:
    raise AssertionError("non-mutex accepted")


def owner():
    """Hold a lock until a timed waiter is suspended, then release once."""
    lock = sc.sc_scoped_lock(mutex)
    assert isinstance(lock, sc.sc_scoped_lock)
    assert sc.sc_scoped_lock.__cpp_name__ == "sc_core::sc_scoped_lock"
    assert lock._mutex_owner is mutex
    assert sc.sc_scoped_lock.lockable_type is sc.sc_mutex_if
    assert mutex.trylock() == 0
    sc.wait(2, sc.SC_NS)
    records.append(("release", str(sc.sc_time_stamp()), lock.release(), sc.sc_scoped_lock.release(lock)))


def waiter():
    """Acquire after contention and check destruction unlocks native storage."""
    sc.wait(1, sc.SC_NS)
    lock = sc.sc_scoped_lock(mutex)
    records.append(("acquired", str(sc.sc_time_stamp()), mutex.trylock() == 0))
    del lock
    gc.collect()
    assert not mutex.active()
    records.append(("destroyed", str(sc.sc_time_stamp()), not mutex.active()))
    mutex.unlock()


handles = [sc.sc_spawn(owner, "owner"), sc.sc_spawn(waiter, "waiter")]
sc.sc_start(3, sc.SC_NS)
assert all(h.terminated() for h in handles)
assert records == [("release", "2 ns", True, False), ("acquired", "2 ns", True), ("destroyed", "2 ns", True)], records
for record in records:
    print("SCOPED_LOCK", *record, sep="\t")


def blocked():
    """Run finally when reset/kill interrupts constructor suspension."""
    try:
        sc.sc_scoped_lock(mutex)
        raise AssertionError("blocked constructor returned")
    finally:
        cleanup.append(str(sc.sc_time_stamp()))


cleanup = []


def control():
    """Reset then kill a blocked constructor while keeping mutex ownership."""
    lock = sc.sc_scoped_lock(mutex)
    child = sc.sc_spawn(blocked, "blocked")
    sc.wait(1, sc.SC_NS)
    child.reset()
    sc.wait(1, sc.SC_NS)
    child.kill()
    assert child.terminated()
    assert lock.release()


controller = sc.sc_spawn(control, "control")
sc.sc_start(3, sc.SC_NS)
assert controller.terminated()
assert cleanup == ["4 ns", "5 ns"], cleanup
print("SCOPED_UNWIND", *cleanup, sep="\t")
sc.sc_stop()
