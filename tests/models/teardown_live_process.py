"""Reject B3 native deletion while a stopped kernel retains a suspended process."""

from amal.eda.systemc import sc_core as sc, tlm
import gc
import weakref


class Owner:
    """Provide a retained analysis callback."""

    def write(self, value):
        """Check copied sample before and after gate rejection."""
        assert value == 17


name = sc.sc_module_name("top")
top = sc.sc_module(name)
owner = Owner()
reference = weakref.ref(owner)
port = sc.sc_port[tlm.tlm_analysis_if[int]]()
port(owner)
native = port.get_interface()
del owner, name
cleanup = []


def worker():
    """Remain suspended when stop occurs; no forced cleanup is requested."""
    try:
        sc.wait(100, sc.SC_NS)
    finally:
        cleanup.append(str(sc.sc_time_stamp()))


handle = sc.sc_spawn(worker, "live_worker")
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
assert not handle.terminated() and cleanup == []
port.__destruct__()
try:
    sc.teardown_interfaces()
except RuntimeError as error:
    assert "process" in str(error)
else:
    raise AssertionError("suspended process teardown accepted")
gc.collect()
assert reference() is not None
native.write(17)
print("LIVE_PROCESS_TEARDOWN_REJECTED 1 ns")
