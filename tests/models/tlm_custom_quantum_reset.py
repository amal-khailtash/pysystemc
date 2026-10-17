"""Verify custom quantum hook reset/kill cleanup across both guarded waits."""

from amal.eda.systemc import sc_core as sc, tlm_utils

records = []
hook_times = []


def quantum():
    """Record hook execution and return the remaining native global quantum."""
    hook_times.append(str(sc.sc_time_stamp()))
    return sc.sc_time(10, sc.SC_NS)


keepers = [tlm_utils.callback_quantumkeeper(quantum), tlm_utils.callback_quantumkeeper(quantum)]
assert type(keepers[0]) is type(keepers[1])
keepers[0].set_global_quantum(sc.sc_time(10, sc.SC_NS))


def worker(index):
    """Suspend in each quantum wait API, retaining keeper and cleanup state."""
    keeper = keepers[index]
    records.append((index, "start", str(sc.sc_time_stamp())))
    keeper.reset()
    try:
        if index:
            keeper.set_and_sync(sc.sc_time(100, sc.SC_NS))
        else:
            keeper.set(sc.sc_time(100, sc.SC_NS))
            keeper.sync()
        raise AssertionError("interrupted quantum wait returned")
    finally:
        records.append((index, "cleanup", str(sc.sc_time_stamp())))


handles = [sc.sc_spawn(lambda index=index: worker(index), f"worker_{index}") for index in range(2)]


def controller():
    """Restart both suspended workers at2ns and terminate them at3ns."""
    sc.wait(2, sc.SC_NS)
    for handle in handles:
        handle.reset()
    sc.wait(1, sc.SC_NS)
    for handle in handles:
        handle.kill()


control = sc.sc_spawn(controller, "controller")
sc.sc_start(4, sc.SC_NS)
for index in range(2):
    assert [record[1:] for record in records if record[0] == index] == [
        ("start", "0 s"),
        ("cleanup", "2 ns"),
        ("start", "2 ns"),
        ("cleanup", "3 ns"),
    ], records
assert all(handle.terminated() for handle in [*handles, control])
assert hook_times == ["0 s", "0 s", "2 ns", "2 ns"], hook_times
sc.sc_stop()
print("CUSTOM_QUANTUM_RESET_OK")
