"""Compare first-party blocking channel reset/kill cleanup with native SystemC."""

import ctypes
from pathlib import Path
import sys
import weakref

from amal.eda.systemc import sc_core as sc, sc_dt, tlm
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_blocking_completion.h"))
NATIVE = cppyy.gbl.pysystemc_blocking_completion
MODE = sys.argv[1] if len(sys.argv) > 1 else "direct"
assert MODE in ("direct", "functor")

Wide = sc_dt.sc_biguint[137]
SignedWide = sc_dt.sc_bigint[137]
CASES = (
    "core_read_value",
    "core_read_output",
    "core_write",
    "tlm_get_value",
    "tlm_get_output",
    "tlm_peek_value",
    "tlm_peek_output",
    "tlm_put",
    "tlm_peek_interface_output",
    "scoped_lock",
    "tlm_get_tag_value",
    "tlm_get_null_tag_value",
    "tlm_peek_tag_value",
    "tlm_peek_null_tag_value",
    "core_bigint_read_value",
    "core_bigint_read_output",
    "core_bigint_write",
)


def wide(value):
    """Return a native wide integer with the requested low word."""
    result = Wide(value)
    result[136] = True
    return result


def signed_wide(value):
    """Return a signed 137-bit value with its sign bit set."""
    result = SignedWide(value)
    result[136] = True
    return result


def value_of(value):
    """Read a scalar or SystemC integer output for lifecycle records."""
    if isinstance(value, ctypes.c_int):
        return value.value
    if hasattr(value, "to_uint64"):
        return int(value.to_uint64())
    return int(value)


module_name = sc.sc_module_name("top")
top = sc.sc_module(module_name)
core_read_value = sc.sc_fifo[int]("core_read_value", 1)
core_read_output = sc.sc_fifo[int]("core_read_output", 1)
core_write = sc.sc_fifo[int]("core_write", 1)
assert core_write.nb_write(17)
core_bigint_read_value = sc.sc_fifo[SignedWide]("core_bigint_read_value", 1)
core_bigint_read_output = sc.sc_fifo[SignedWide]("core_bigint_read_output", 1)
core_bigint_write = sc.sc_fifo[SignedWide]("core_bigint_write", 1)
assert core_bigint_write.nb_write(signed_wide(17))
tlm_get_value = tlm.tlm_fifo[Wide]("tlm_get_value", 1)
tlm_get_output = tlm.tlm_fifo[Wide]("tlm_get_output", 1)
tlm_peek_value = tlm.tlm_fifo[Wide]("tlm_peek_value", 1)
tlm_peek_output = tlm.tlm_fifo[Wide]("tlm_peek_output", 1)
tlm_put = tlm.tlm_fifo[Wide]("tlm_put", 1)
tlm_get_tag_value = tlm.tlm_fifo[Wide]("tlm_get_tag_value", 1)
tlm_get_null_tag_value = tlm.tlm_fifo[Wide]("tlm_get_null_tag_value", 1)
tlm_peek_tag_value = tlm.tlm_fifo[Wide]("tlm_peek_tag_value", 1)
tlm_peek_null_tag_value = tlm.tlm_fifo[Wide]("tlm_peek_null_tag_value", 1)
tlm_peek_interface = tlm.tlm_fifo[Wide]("tlm_peek_interface", 1)
assert tlm_put.nb_put(wide(17))
peek_port = sc.sc_port[tlm.tlm_blocking_peek_if[Wide]]("peek_port")
peek_port.bind(tlm_peek_interface)
mutex = sc.sc_mutex("mutex")
del module_name

channels = {
    "core_read_value": core_read_value,
    "core_read_output": core_read_output,
    "core_write": core_write,
    "core_bigint_read_value": core_bigint_read_value,
    "core_bigint_read_output": core_bigint_read_output,
    "core_bigint_write": core_bigint_write,
    "tlm_get_value": tlm_get_value,
    "tlm_get_output": tlm_get_output,
    "tlm_peek_value": tlm_peek_value,
    "tlm_peek_output": tlm_peek_output,
    "tlm_put": tlm_put,
    "tlm_get_tag_value": tlm_get_tag_value,
    "tlm_get_null_tag_value": tlm_get_null_tag_value,
    "tlm_peek_tag_value": tlm_peek_tag_value,
    "tlm_peek_null_tag_value": tlm_peek_null_tag_value,
    "tlm_peek_interface_output": peek_port,
    "scoped_lock": mutex,
}
records = {case: [] for case in CASES}
handles = {}
tag_refs = []


def blocking_tag():
    """Return a temporary tag while tracking its owner through suspension."""
    tag = tlm.tlm_tag[Wide]()
    tag_refs.append(weakref.ref(tag))
    return tag


def make_native_operation(case, channel, output, sample, tag_owner):
    """Build C++-only work that borrows this route's native storage."""
    if case == "core_read_value":
        return NATIVE.core_fifo_read_value[int](channel)
    if case == "core_read_output":
        return NATIVE.core_fifo_read_output[int](channel, output)
    if case == "core_write":
        return NATIVE.core_fifo_write[int](channel, sample)
    if case == "core_bigint_read_value":
        return NATIVE.core_fifo_read_value[SignedWide](channel)
    if case == "core_bigint_read_output":
        return NATIVE.core_fifo_read_output[SignedWide](channel, output)
    if case == "core_bigint_write":
        return NATIVE.core_fifo_write[SignedWide](channel, sample)
    if case in ("tlm_get_value", "tlm_get_tag_value", "tlm_get_null_tag_value"):
        tag = tag_owner if tag_owner is not None else cppyy.nullptr
        return NATIVE.tlm_fifo_get_value[Wide](channel, tag)
    if case == "tlm_get_output":
        return NATIVE.tlm_fifo_get_output[Wide](channel, output)
    if case in ("tlm_peek_value", "tlm_peek_tag_value", "tlm_peek_null_tag_value"):
        tag = tag_owner if tag_owner is not None else cppyy.nullptr
        return NATIVE.tlm_fifo_peek_value[Wide](channel, tag)
    if case == "tlm_peek_output":
        return NATIVE.tlm_fifo_peek_output[Wide](channel, output)
    if case == "tlm_peek_interface_output":
        return NATIVE.tlm_interface_peek_output[Wide](channel.get_interface(), output)
    if case == "tlm_put":
        return NATIVE.tlm_fifo_put[Wide](channel, sample)
    if case == "scoped_lock":
        return NATIVE.scoped_lock(channel)
    raise AssertionError(f"no native operation factory for {case}")


def worker(case):
    """Exercise one blocking route and preserve borrowed values in cleanup."""
    scalar_core = case in ("core_read_value", "core_read_output", "core_write")
    output = ctypes.c_int(71) if scalar_core else signed_wide(71) if case.startswith("core_bigint") else wide(71)
    sample = (
        99
        if case == "core_write"
        else signed_wide(99)
        if case == "core_bigint_write"
        else wide(99)
        if case == "tlm_put"
        else None
    )
    tag_owner = blocking_tag() if MODE == "functor" and case in ("tlm_get_tag_value", "tlm_peek_tag_value") else None
    channel = channels[case]
    records[case].append(("start", str(sc.sc_time_stamp())))
    try:
        if MODE == "functor":
            operation = make_native_operation(case, channel, output, sample, tag_owner)
            assert sc.native_blocking_call(operation) is None
        elif case == "core_read_value":
            channel.read()
        elif case == "core_read_output":
            channel.read(output)
        elif case == "core_write":
            channel.write(sample)
        elif case == "tlm_get_value":
            channel.get()
        elif case == "tlm_get_output":
            channel.get(output)
        elif case == "tlm_peek_value":
            channel.peek()
        elif case == "tlm_peek_output":
            channel.peek(output)
        elif case == "tlm_put":
            channel.put(sample)
        elif case == "tlm_peek_interface_output":
            channel.get_interface().peek(output)
        elif case == "tlm_get_tag_value":
            channel.get(blocking_tag())
        elif case == "tlm_get_null_tag_value":
            channel.get(cppyy.nullptr)
        elif case == "tlm_peek_tag_value":
            channel.peek(blocking_tag())
        elif case == "tlm_peek_null_tag_value":
            channel.peek(cppyy.nullptr)
        elif case == "core_bigint_read_value":
            channel.read()
        elif case == "core_bigint_read_output":
            channel.read(output)
        elif case == "core_bigint_write":
            channel.write(sample)
        else:
            sc.sc_scoped_lock(channel)
        raise AssertionError(f"{case} returned without data, space or lock ownership")
    finally:
        output_value = value_of(output)
        input_value = value_of(sample) if sample is not None else 0
        assert output_value == 71, (case, output_value)
        wide_storage = not scalar_core
        if wide_storage:
            assert bool(output[136]), (case, "output high bit changed")
        assert input_value == (99 if sample is not None else 0), (case, input_value)
        if sample is not None and case in ("tlm_put", "core_bigint_write"):
            assert bool(sample[136]), "put input high bit changed"
        records[case].append(("cleanup", str(sc.sc_time_stamp()), output_value, input_value))


def hold_mutex():
    """Own the mutex until every scoped-lock reset and kill has completed."""
    assert mutex.lock() == 0
    sc.wait(4, sc.SC_NS)
    assert mutex.unlock() == 0


holder = sc.sc_spawn(hold_mutex, "holder")
for case in CASES:
    options = sc.sc_spawn_options()
    options.set_stack_size(2 * 1024 * 1024)
    handles[case] = sc.sc_spawn(lambda case=case: worker(case), case, options)


def control():
    """Reset every blocked operation at two ns and kill each restart at three."""
    sc.wait(1, sc.SC_NS)
    assert len(tag_refs) == 2 and all(reference() is not None for reference in tag_refs), tag_refs
    sc.wait(1, sc.SC_NS)
    for handle in handles.values():
        handle.reset()
    sc.wait(1, sc.SC_NS)
    for handle in handles.values():
        handle.kill()


controller = sc.sc_spawn(control, "controller")
sc.sc_start(5, sc.SC_NS)
for case in CASES:
    assert records[case] == [
        ("start", "0 s"),
        ("cleanup", "2 ns", 71, 99 if case in ("core_write", "core_bigint_write", "tlm_put") else 0),
        ("start", "2 ns"),
        ("cleanup", "3 ns", 71, 99 if case in ("core_write", "core_bigint_write", "tlm_put") else 0),
    ], (case, records[case])
assert all(handle.terminated() for handle in (*handles.values(), holder, controller))
assert core_write.num_available() == 1 and core_write.read() == 17
signed_seed = core_bigint_write.read()
assert signed_seed.to_uint64() == 17 and bool(signed_seed[136])
retained_put = wide(-1)
assert tlm_put.used() == 1 and tlm_put.nb_get(retained_put)
assert value_of(retained_put) == 17
assert mutex.trylock() == 0 and mutex.unlock() == 0
sc.sc_stop()
for case in CASES:
    for record in records[case]:
        print("BLOCKING_REFERENCE", case, *record, sep="|")
print("SC_CORE_BLOCKING_COMPLETION_OK")
