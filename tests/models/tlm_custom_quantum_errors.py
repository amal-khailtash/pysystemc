"""Verify custom hook failures after waits and native global-boundary computation."""

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils

mode = "valid"
hooks = []


def compute():
    """Fail once or compute the native remaining quantum at the current time."""
    hooks.append(str(sc.sc_time_stamp()))
    if mode == "error":
        raise ValueError("after wait")
    if mode == "invalid":
        return None
    return tlm.tlm_global_quantum.instance().compute_local_quantum()


keeper = tlm_utils.callback_quantumkeeper(compute)
keeper.set_global_quantum(sc.sc_time(5, sc.SC_NS))
keeper.reset()


def worker():
    """Catch both post-wait failures, recover and synchronize at global boundaries."""
    global mode
    for operation, bad, expected in (
        (keeper.sync, "error", ValueError),
        (lambda: keeper.set_and_sync(sc.sc_time(5, sc.SC_NS)), "invalid", TypeError),
    ):
        keeper.set(sc.sc_time(5, sc.SC_NS))
        mode = bad
        try:
            operation()
        except expected:
            pass
        else:
            raise AssertionError("post-wait error lost")
        assert keeper.get_local_time() == sc.SC_ZERO_TIME
        mode = "valid"
        keeper.reset()
    assert sc.sc_time_stamp() == sc.sc_time(10, sc.SC_NS)
    sc.wait(2, sc.SC_NS)
    keeper.reset()
    keeper.set(sc.sc_time(2, sc.SC_NS))
    assert not keeper.need_sync()
    keeper.inc(sc.sc_time(1, sc.SC_NS))
    assert keeper.need_sync()
    keeper.sync()
    assert sc.sc_time_stamp() == sc.sc_time(15, sc.SC_NS)
    assert keeper.get_local_time() == sc.SC_ZERO_TIME


handle = sc.sc_spawn(worker, "worker")
sc.sc_start(16, sc.SC_NS)
assert handle.terminated()
assert hooks == ["0 s", "5 ns", "5 ns", "10 ns", "10 ns", "12 ns", "15 ns"], hooks
sc.sc_stop()
print("CUSTOM_QUANTUM_ERRORS_OK")
