"""Verify native mutex/semaphore reset and kill unwind Python cleanup."""

import sys
from amal.eda.systemc import sc_core as sc

kind, access = sys.argv[1:]
name = sc.sc_module_name("top")
top = sc.sc_module(name)
channel = sc.sc_mutex("channel") if kind == "mutex" else sc.sc_semaphore("channel", 0)
interface = sc.sc_mutex_if if kind == "mutex" else sc.sc_semaphore_if
port = sc.sc_port[interface]("port")
port(channel)
target = channel if access == "direct" else port.get_interface()
del name
records = []


def holder():
    """Keep the native mutex owned while the contender is reset and killed."""
    assert channel.lock() == 0
    sc.wait(4, sc.SC_NS)
    assert channel.unlock() == 0


if kind == "mutex":
    owner = sc.sc_spawn(holder, "holder")


def worker():
    """Suspend in a borrowed native operation and record Python cleanup."""
    records.append(("start", str(sc.sc_time_stamp())))
    try:
        target.lock() if kind == "mutex" else target.wait()
        raise AssertionError("blocked operation returned")
    finally:
        records.append(("cleanup", str(sc.sc_time_stamp())))


handle = sc.sc_spawn(worker, "worker")


def control():
    """Reset at two ns and kill at three ns without releasing the resource."""
    sc.wait(2, sc.SC_NS)
    handle.reset()
    sc.wait(1, sc.SC_NS)
    handle.kill()


controller = sc.sc_spawn(control, "control")
sc.sc_start(5, sc.SC_NS)
assert records == [("start", "0 s"), ("cleanup", "2 ns"), ("start", "2 ns"), ("cleanup", "3 ns")], records
assert handle.terminated() and controller.terminated()
if kind == "mutex":
    assert owner.terminated()
    assert channel.trylock() == 0
    assert channel.unlock() == 0
else:
    assert channel.get_value() == 0
    assert channel.post() == 0
    assert channel.trywait() == 0
sc.sc_stop()
for action, time in records:
    print(f"SYNC_RESET_REFERENCE {action} {time}")
print("NATIVE_SYNC_GUARDED_RESET_OK")
