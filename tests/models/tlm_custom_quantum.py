"""Check custom local quantum dispatch, errors, ownership and synchronization."""

import gc
import weakref

from amal.eda.systemc import sc_core as sc, tlm_utils


class Compute:
    """Compute a bounded local quantum and inject callback failures."""

    def __init__(self):
        """Initialize callback mode and recorded hook times."""
        self.mode = "valid"
        self.calls = []

    def __call__(self):
        """Return native time or raise the selected contract violation."""
        self.calls.append(str(sc.sc_time_stamp()))
        if self.mode == "error":
            raise ValueError("quantum failure")
        if self.mode == "invalid":
            return 3
        return sc.sc_time(3, sc.SC_NS)


compute = Compute()
reference = weakref.ref(compute)
try:
    tlm_utils.callback_quantumkeeper(None)
except TypeError:
    pass
else:
    raise AssertionError("noncallable accepted")
keeper = tlm_utils.callback_quantumkeeper(compute)
keeper.set_global_quantum(sc.sc_time(10, sc.SC_NS))
for mode, exception in (("invalid", TypeError), ("error", ValueError)):
    compute.mode = mode
    keeper.set(sc.sc_time(1, sc.SC_NS))
    try:
        keeper.reset()
    except exception:
        pass
    else:
        raise AssertionError("bad hook accepted")
    assert keeper.get_local_time() == sc.SC_ZERO_TIME
compute.mode = "valid"
keeper.reset()
del compute
gc.collect()
assert reference() is not None


def worker():
    """Exercise both guarded waits and exact custom synchronization thresholds."""
    keeper.inc(sc.sc_time(2, sc.SC_NS))
    assert not keeper.need_sync()
    keeper.inc(sc.sc_time(1, sc.SC_NS))
    assert keeper.need_sync()
    keeper.sync()
    assert sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
    assert keeper.get_local_time() == sc.SC_ZERO_TIME
    print("CUSTOM_QUANTUM", sc.sc_time_stamp())
    keeper.set_and_sync(sc.sc_time(2, sc.SC_NS))
    assert sc.sc_time_stamp() == sc.sc_time(3, sc.SC_NS)
    keeper.set_and_sync(sc.sc_time(3, sc.SC_NS))
    assert sc.sc_time_stamp() == sc.sc_time(6, sc.SC_NS)
    print("CUSTOM_QUANTUM", sc.sc_time_stamp())


handle = sc.sc_spawn(worker, "worker")
sc.sc_start(7, sc.SC_NS)
assert handle.terminated()
assert reference().calls[-3:] == ["0 s", "3 ns", "6 ns"]
sc.sc_stop()
keeper = None
gc.collect()
assert reference() is None
print("CUSTOM_QUANTUM_OK")
