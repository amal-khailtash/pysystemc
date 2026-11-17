"""Exercise FIFO adaptation stability and malformed blocking read recovery."""

from amal.eda.systemc import sc_core as sc, tlm
from amal.eda.systemc.tlm._fifo import _pythonize_fifo


fifo_type = tlm.tlm_fifo[int]
method_identities = {name: getattr(fifo_type, name) for name in ("get", "peek", "put")}
# Explicitly reinstall the native pythonizer, then separately select the same
# template repeatedly. The first checks the adapter's idempotence guard; the
# second checks the public template cache path.
_pythonize_fifo(fifo_type, "tlm_fifo")
assert all(getattr(fifo_type, name) is method for name, method in method_identities.items())
for _ in range(4):
    repeated_type = tlm.tlm_fifo[int]
    assert repeated_type is fifo_type
    assert all(getattr(repeated_type, name) is method for name, method in method_identities.items())

module_name = sc.sc_module_name("top")
top = sc.sc_module(module_name)
del module_name
fifo = fifo_type("edge_fifo", 1)
assert fifo.nb_put(314)
tag = tlm.tlm_tag[int]()
observations = {}


def spawn_thread(callback, name):
    """Spawn a SystemC thread with a Python-compatible coroutine stack."""
    options = sc.sc_spawn_options()
    options.set_stack_size(2 * 1024 * 1024)
    return sc.sc_spawn(callback, name, options)


def reader():
    """Reject malformed calls without changing time or queued data."""
    start_time = str(sc.sc_time_stamp())
    for name, operation in (("get", fifo.get), ("peek", fifo.peek)):
        try:
            operation(tag, tag)
        except TypeError:
            pass
        else:
            raise AssertionError(f"{name} accepted two tag arguments")
        assert str(sc.sc_time_stamp()) == start_time
        assert fifo.used() == 1
        observations[name + "_time"] = str(sc.sc_time_stamp())

    sc.wait(1, sc.SC_NS)
    peeked = fifo.peek(tag)
    used_after_peek = fifo.used()
    received = fifo.get(tag)
    observations.update(
        start_time=start_time,
        valid_time=str(sc.sc_time_stamp()),
        peeked=int(peeked),
        received=int(received),
        used_after_peek=used_after_peek,
        used_after_get=fifo.used(),
    )


reader_handle = spawn_thread(reader, "reader")
sc.sc_start(2, sc.SC_NS)
assert reader_handle.terminated()
assert observations == {
    "get_time": "0 s",
    "peek_time": "0 s",
    "start_time": "0 s",
    "valid_time": "1 ns",
    "peeked": 314,
    "received": 314,
    "used_after_peek": 1,
    "used_after_get": 0,
}
print(
    "TLM_ADAPTER_FIFO",
    observations["get_time"],
    observations["peek_time"],
    observations["valid_time"],
    observations["peeked"],
    observations["received"],
    observations["used_after_peek"],
    observations["used_after_get"],
)
