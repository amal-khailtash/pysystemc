"""Exercise guarded Python blocking operations on native TLM FIFOs."""

import gc

from amal.eda.systemc import sc_core as sc, sc_dt, tlm


VALUE = sc_dt.sc_biguint[191]
module_name = sc.sc_module_name("top")
top = sc.sc_module(module_name)
del module_name
fifo = tlm.tlm_fifo[VALUE]("full_fifo", 1)
cleanup_get_fifo = tlm.tlm_fifo[VALUE]("cleanup_get_fifo", 1)
cleanup_get_output_fifo = tlm.tlm_fifo[VALUE]("cleanup_get_output_fifo", 1)
cleanup_peek_fifo = tlm.tlm_fifo[VALUE]("cleanup_peek_fifo", 1)
cleanup_peek_output_fifo = tlm.tlm_fifo[VALUE]("cleanup_peek_output_fifo", 1)
cleanup_put_fifo = tlm.tlm_fifo[VALUE]("cleanup_put_fifo", 1)


def wide(low):
    """Construct a 191-bit value with a distinguishing high bit set."""
    value = VALUE(low)
    value[190] = True
    return value


def pair(value):
    """Return the low word and arbitrary-width high bit for comparison."""
    return int(value.to_uint64()), int(bool(value[190]))


def spawn_thread(callback, name):
    """Spawn a Python process with a2MiB stack for coroutine compatibility."""
    options = sc.sc_spawn_options()
    options.set_stack_size(2 * 1024 * 1024)
    return sc.sc_spawn(callback, name, options)


seed = wide(101)
assert fifo.nb_put(seed)
del seed
put_seed = wide(606)
assert cleanup_put_fifo.nb_put(put_seed)
del put_seed
tag = tlm.tlm_tag[VALUE]()
values = {}
records = []


def writer():
    """Block on a full native FIFO then release the local input value."""
    records.append(("put_start", str(sc.sc_time_stamp())))
    value = wide(202)
    fifo.put(value)
    records.append(("put_done", str(sc.sc_time_stamp())))
    del value
    gc.collect()


def reader():
    """Exercise blocked get/peek overloads without consuming on peek."""
    sc.wait(1, sc.SC_NS)
    first = fifo.get(tag)
    values["tagged_get"] = first
    records.append(("tagged_get", pair(first), str(sc.sc_time_stamp())))

    get_output = wide(404)
    assert fifo.get(get_output) is None
    values["get_output"] = get_output
    records.append(("get_output", pair(get_output), str(sc.sc_time_stamp())))

    peeked = fifo.peek(tag)
    values["tagged_peek"] = peeked
    assert pair(peeked) == (303, 1)
    used_after_value_peek = fifo.used()

    peek_output = wide(505)
    assert fifo.peek(peek_output) is None
    values["peek_output"] = peek_output
    used_after_output_peek = fifo.used()

    next_value = fifo.get(tag)
    values["tagged_get_after_peek"] = next_value
    assert pair(next_value) == (303, 1)

    waiting_peek_output = wide(707)
    assert fifo.peek(waiting_peek_output) is None
    values["waiting_peek_output"] = waiting_peek_output
    used_after_waiting_peek = fifo.used()

    last_get_output = wide(808)
    assert fifo.get(last_get_output) is None
    values["last_get_output"] = last_get_output
    records.append(
        (
            "output_forms",
            pair(peek_output),
            pair(waiting_peek_output),
            pair(last_get_output),
            used_after_value_peek,
            used_after_output_peek,
            used_after_waiting_peek,
            fifo.used(),
            str(sc.sc_time_stamp()),
        )
    )


def make_cleanup_worker(label, channel, operation):
    """Create a reset/kill worker and retain its mutable output sentinel."""

    def worker():
        """Block in one native interface operation and record Python cleanup."""
        output = wide(909)
        records.append((label + "_start", str(sc.sc_time_stamp())))
        try:
            if operation == "get_value":
                channel.get(tag)
            elif operation == "get_output":
                channel.get(output)
            elif operation == "peek_value":
                channel.peek(tag)
            elif operation == "peek_output":
                channel.peek(output)
            elif operation == "put":
                channel.put(wide(1001))
            raise AssertionError(f"blocked {operation} returned without data or capacity")
        finally:
            records.append((label + "_finally", str(sc.sc_time_stamp()), pair(output)))

    return worker


cleanup_cases = (
    ("get_value", cleanup_get_fifo, "get_value"),
    ("get_output", cleanup_get_output_fifo, "get_output"),
    ("peek_value", cleanup_peek_fifo, "peek_value"),
    ("peek_output", cleanup_peek_output_fifo, "peek_output"),
    ("reset_put", cleanup_put_fifo, "put"),
)
cleanup_handles = {
    label: spawn_thread(make_cleanup_worker(label, channel, operation), label)
    for label, channel, operation in cleanup_cases
}
put_handle = spawn_thread(writer, "writer")
read_handle = spawn_thread(reader, "reader")


def delayed_writer(delay, low):
    """Return a producer that inserts one temporary 191-bit value later."""

    def produce():
        """Wait for the scheduled time, copy the value, and release it."""
        sc.wait(delay, sc.SC_NS)
        value = wide(low)
        fifo.put(value)
        records.append(("delayed_put", low, str(sc.sc_time_stamp())))
        del value
        gc.collect()

    return produce


delayed_handles = [
    spawn_thread(delayed_writer(2, 303), "delayed_writer_303"),
    spawn_thread(delayed_writer(3, 404), "delayed_writer_404"),
]


def controller():
    """Reset blocked FIFO operations at1ns and kill restarts at2ns."""
    sc.wait(1, sc.SC_NS)
    for handle in cleanup_handles.values():
        handle.reset()
    sc.wait(1, sc.SC_NS)
    for handle in cleanup_handles.values():
        handle.kill()


control_handle = spawn_thread(controller, "controller")
sc.sc_start(4, sc.SC_NS)

for label in cleanup_handles:
    start = label + "_start"
    cleanup = label + "_finally"
    assert [(record[0], record[1]) for record in records if record[0] in (start, cleanup)] == [
        (start, "0 s"),
        (cleanup, "1 ns"),
        (start, "1 ns"),
        (cleanup, "2 ns"),
    ], (label, records)
    assert all(record[2] == (909, 1) for record in records if record[0] == label + "_finally"), (label, records)
assert ("put_start", "0 s") in records
assert ("put_done", "1 ns") in records
assert ("tagged_get", (101, 1), "1 ns") in records
assert ("get_output", (202, 1), "1 ns") in records
assert ("delayed_put", 303, "2 ns") in records and ("delayed_put", 404, "3 ns") in records
assert (
    "output_forms",
    (303, 1),
    (404, 1),
    (404, 1),
    1,
    1,
    1,
    0,
    "3 ns",
) in records
assert all(
    handle.terminated()
    for handle in (*cleanup_handles.values(), put_handle, read_handle, *delayed_handles, control_handle)
)

gc.collect()
assert pair(values["tagged_get"]) == (101, 1)
assert pair(values["get_output"]) == (202, 1)
assert pair(values["tagged_peek"]) == (303, 1)
assert pair(values["peek_output"]) == (303, 1)
assert pair(values["tagged_get_after_peek"]) == (303, 1)
assert pair(values["waiting_peek_output"]) == (404, 1)
assert pair(values["last_get_output"]) == (404, 1)
assert sc.sc_time_stamp() == sc.sc_time(4, sc.SC_NS)
sc.sc_stop()
print("TLM_FIFO_BLOCKING_PUBLIC_OK")
