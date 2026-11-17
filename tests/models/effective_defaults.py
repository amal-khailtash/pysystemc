"""Assert selected effective defaults in one fresh native kernel per invocation."""

import ctypes
import sys

from amal.eda.systemc import sc_core as sc


def run_start(shape, policy):
    """Assert an empty kernel advances seven ns unless starvation exit is explicit.

    Args:
        shape: ``time``, ``int``, or ``double`` overload shape.
        policy: ``omitted``, ``run``, or ``exit`` policy selection.

    Returns:
        None; prints the final timestamp in native resolution ticks.

    Raises:
        AssertionError: The selected default has different time behavior.
    """
    arguments = (sc.sc_time(7, sc.SC_NS),) if shape == "time" else (7.0 if shape == "double" else 7, sc.SC_NS)
    if policy != "omitted":
        arguments += (sc.SC_RUN_TO_TIME if policy == "run" else sc.SC_EXIT_ON_STARVATION,)
    sc.sc_start(*arguments)
    assert sc.sc_time_stamp() == sc.sc_time(0 if policy == "exit" else 7, sc.SC_NS)
    print("DEFAULT_START", sc.sc_time_stamp().value(), sep="\t")


def run_fifo():
    """Compare default and explicit 16-slot FIFOs through full, read and refill.

    Both channels remain alive until simulation control has finished. Only
    nonblocking operations run; no coroutine or blocking FIFO support is tested.
    Returns None and prints capacity, ordered-value checksum and recovered space.
    Raises AssertionError if capacity, values or update timing differs.
    """
    channels = (sc.sc_fifo[int](), sc.sc_fifo[int](16))
    for channel in channels:
        assert channel.num_free() == 16 and channel.num_available() == 0
        for value in range(16):
            assert channel.nb_write(value)
        assert channel.num_free() == 0 and not channel.nb_write(99)
    sc.sc_start(sc.SC_ZERO_TIME)
    for channel in channels:
        assert channel.num_available() == 16
        output = ctypes.c_int(-1)
        values = []
        for value in range(16):
            assert channel.nb_read(output) and output.value == value
            values.append(output.value)
        assert not channel.nb_read(output) and output.value == 15
        assert sum(values) == 120 and channel.num_available() == 0
    sc.sc_start(sc.SC_ZERO_TIME)
    for channel in channels:
        assert channel.num_free() == 16 and channel.nb_write(42)
    print("DEFAULT_FIFO", 16, 120, 16, sep="\t")


def run_queue():
    """Compare generated and explicit module names through queued method delivery.

    The module retains its callbacks; local queues and records outlive all
    callbacks. Calls and callbacks execute on the simulation thread with the
    Python GIL managed by the existing bridge. Queues are destroyed only after
    sc_stop; no kernel globals are reset. Returns None and prints time/lifetime
    evidence. Raises AssertionError on naming, callback or teardown mismatch.
    """
    generated = sc.sc_event_queue()
    explicit_name = sc.sc_module_name(sc.sc_gen_unique_name("event_queue"))
    explicit = sc.sc_event_queue(explicit_name)
    del explicit_name
    names = [str(queue.name()) for queue in (generated, explicit)]
    assert names == ["event_queue_0", "event_queue_1"]
    module_name = sc.sc_module_name("observer")
    module = sc.sc_module(module_name)
    records = ([], [])

    def observe_generated():
        """Retain the timestamp and delta of the generated queue's delivery."""
        records[0].append((sc.sc_time_stamp().value(), sc.sc_delta_count()))

    def observe_explicit():
        """Retain the timestamp and delta of the explicitly named queue's delivery."""
        records[1].append((sc.sc_time_stamp().value(), sc.sc_delta_count()))

    for queue, callback in ((generated, observe_generated), (explicit, observe_explicit)):
        module.sc_method(callback.__name__, callback)
        module.sensitive(queue.default_event())
        module.dont_initialize()
    del module_name
    for queue in (generated, explicit):
        queue.notify(sc.sc_time(1, sc.SC_NS))
        queue.notify(sc.sc_time(3, sc.SC_NS))
    sc.sc_start(4, sc.SC_NS)
    assert records[0] == records[1]
    assert [time for time, delta in records[0]] == [1000, 3000]
    assert records[0][0][1] < records[0][1][1]
    sc.sc_stop()
    generated.__destruct__()
    explicit.__destruct__()
    assert all(not sc.sc_find_object(name) for name in names)
    print("DEFAULT_QUEUE", 1000, 3000, len(records[0]), 1, sep="\t")


if __name__ == "__main__":
    scenario, selected_policy = sys.argv[1:]
    if scenario == "fifo":
        run_fifo()
    elif scenario == "queue":
        run_queue()
    else:
        run_start(scenario, selected_policy)
