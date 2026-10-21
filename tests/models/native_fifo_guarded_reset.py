"""Verify direct native FIFO reset/kill preserves Python cleanup and output."""

from amal.eda.systemc import sc_core as sc
import ctypes
import sys

write_mode = len(sys.argv) > 1 and sys.argv[1] == "write"
access = sys.argv[2] if len(sys.argv) > 2 else "direct"
port_mode = access == "port"
name = sc.sc_module_name("top")
top = sc.sc_module(name)
fifo = sc.sc_fifo[int]("fifo", 1)
if write_mode:
    assert fifo.nb_write(17)
if port_mode:
    port = sc.sc_port[sc.sc_fifo_out_if[int] if write_mode else sc.sc_fifo_in_if[int]]()
    port(fifo)
    target = port.get_interface()
elif access == "convenience":
    port = (sc.sc_fifo_out[int] if write_mode else sc.sc_fifo_in[int])("port")
    port(fifo)
    target = port
else:
    target = fifo
del name
records = []
output = ctypes.c_int(71)


def worker():
    """Block inside native FIFO storage with caller-owned output."""
    records.append(("start", str(sc.sc_time_stamp())))
    try:
        target.write(23) if write_mode else target.read(output)
        raise AssertionError("empty read returned")
    finally:
        assert output.value == 71
        records.append(("cleanup", str(sc.sc_time_stamp())))


handle = sc.sc_spawn(worker, "worker")


def control():
    """Reset then kill the native FIFO consumer."""
    sc.wait(2, sc.SC_NS)
    handle.reset()
    sc.wait(1, sc.SC_NS)
    handle.kill()


controller = sc.sc_spawn(control, "control")
sc.sc_start(4, sc.SC_NS)
assert records == [("start", "0 s"), ("cleanup", "2 ns"), ("start", "2 ns"), ("cleanup", "3 ns")], records
assert handle.terminated() and controller.terminated()
sc.sc_stop()
print("NATIVE_FIFO_GUARDED_RESET_OK")
