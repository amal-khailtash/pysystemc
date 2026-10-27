"""Exercise bounded GC ownership edges for structural channels and TLM calls."""

import gc
import sys
from typing import Protocol
import weakref

from amal.eda.systemc import sc_core as sc, sc_dt, tlm, tlm_utils
import cppyy


def structural_owner_lifetime():
    """Retain a generic channel owner through reset/error and release it after stop."""

    @sc.interface(methods={"acquire_resource": ("int", ""), "inspect_resource": ("int", "")})
    class ChannelContract(Protocol):
        """Declare the exact blocking and inspection signatures for the port."""

    records = []

    class Owner:
        """Supply one blocking and one recoverable structural callback."""

        def __init__(self):
            """Start with a valid inspection callback."""
            self.fail = False

        def acquire_resource(self):
            """Suspend so reset and kill unwind the structural callback frame."""
            try:
                sc.wait(100, sc.SC_NS)
            finally:
                records.append(str(sc.sc_time_stamp()))
            return 0

        def inspect_resource(self):
            """Raise one configured error, then return a stable recovered value."""
            if self.fail:
                raise ValueError("configured channel inspection failure")
            return 29

    name = sc.sc_module_name("f6_top")
    top = sc.sc_module(name)
    assert top.name() == "f6_top"
    owner = Owner()
    owner_ref = weakref.ref(owner)
    port = sc.sc_port[ChannelContract]("f6_port")
    port(owner)
    native = port.get_interface()
    owner.fail = True
    try:
        native.inspect_resource()
    except ValueError as error:
        assert "configured channel inspection failure" in str(error)
    else:
        raise AssertionError("structural callback error was lost")
    owner.fail = False
    assert native.inspect_resource() == 29
    del owner, name
    gc.collect()
    assert owner_ref() is not None

    def worker():
        """Invoke the retained blocking callback until the controller resets it."""
        native.acquire_resource()
        raise AssertionError("interrupted structural callback returned")

    handle = sc.sc_spawn(worker, "f6_worker")

    def controller():
        """Reset and kill the callback process at distinct native times."""
        sc.wait(2, sc.SC_NS)
        handle.reset()
        sc.wait(1, sc.SC_NS)
        handle.kill()

    control = sc.sc_spawn(controller, "f6_controller")
    sc.sc_start(4, sc.SC_NS)
    assert records == ["2 ns", "3 ns"], records
    assert handle.terminated() and control.terminated()
    assert owner_ref() is not None
    sc.sc_stop()

    released = sc.release_interface_callbacks()
    assert released == 1, released
    gc.collect()
    assert owner_ref() is None
    try:
        native.inspect_resource()
    except RuntimeError as error:
        assert "released" in str(error).lower()
    else:
        raise AssertionError("released structural callback remained callable")
    print("F6_STRUCTURAL_OWNER_OK reset=2ns kill=3ns released=1")


def fifo_call_frame_lifetime():
    """Release values, tags and outputs after every blocking FIFO overload returns."""
    value_type = sc_dt.sc_biguint[137]
    name = sc.sc_module_name("f6_fifo_top")
    top = sc.sc_module(name)
    assert top.name() == "f6_fifo_top"
    fifo = tlm.tlm_fifo[value_type]("f6_fifo", 1)
    seed = value_type(11)
    seed[136] = True
    assert fifo.nb_put(seed)
    del seed

    def make_value(low):
        """Create a wide sample with a set high bit for boundary checking."""
        value = value_type(low)
        value[136] = True
        return value

    references = {}
    observations = []

    class ErrorOwner:
        """Raise a callback error before the adapter writes to caller output."""

        def get(self):
            """Raise an ordinary callback error for output-preservation checks."""
            raise ValueError("configured TLM get failure")

    error_owner = ErrorOwner()
    error_owner_ref = weakref.ref(error_owner)
    error_port = sc.sc_port[tlm.tlm_blocking_get_if[value_type]]("f6_error_port")
    error_port(error_owner)
    error_interface = error_port.get_interface()
    del error_owner

    def check_error_output_lifetime():
        """Check traceback-held output storage is released after the error frame."""
        output = make_value(71)
        output_ref = weakref.ref(output)
        try:
            error_interface.get(output)
        except ValueError as error:
            assert "configured TLM get failure" in str(error)
            failure_traceback = error.__traceback__
        else:
            raise AssertionError("TLM get callback error was lost")
        assert int(output.to_uint64()) == 71 and bool(output[136])
        del output, failure_traceback
        gc.collect()
        assert output_ref() is None
        assert error_owner_ref() is not None

    check_error_output_lifetime()

    def writer():
        """Block in put, then exercise tagged values and caller-owned outputs."""
        tag = tlm.tlm_tag[value_type]()
        references["tag"] = weakref.ref(tag)
        sample = make_value(23)
        references["input"] = weakref.ref(sample)
        fifo.put(sample)
        observations.append(("put", int(sample.to_uint64()), str(sc.sc_time_stamp())))
        del sample
        gc.collect()

        get_value = fifo.get(tag)
        references["get_value"] = weakref.ref(get_value)
        observations.append(("get_value", int(get_value.to_uint64()), str(sc.sc_time_stamp())))
        del get_value
        gc.collect()

        second_sample = make_value(31)
        references["second_input"] = weakref.ref(second_sample)
        fifo.put(second_sample)
        observations.append(("second_put", int(second_sample.to_uint64()), str(sc.sc_time_stamp())))
        del second_sample
        gc.collect()

        peek_value = fifo.peek(tag)
        references["peek_value"] = weakref.ref(peek_value)
        observations.append(("peek_value", int(peek_value.to_uint64()), str(sc.sc_time_stamp())))
        del peek_value
        gc.collect()

        peek_output = make_value(71)
        references["peek_output"] = weakref.ref(peek_output)
        assert fifo.peek(peek_output) is None
        observations.append(("peek_output", int(peek_output.to_uint64()), str(sc.sc_time_stamp())))
        del peek_output
        gc.collect()

        get_output = make_value(73)
        references["get_output"] = weakref.ref(get_output)
        assert fifo.get(get_output) is None
        observations.append(("get_output", int(get_output.to_uint64()), str(sc.sc_time_stamp())))
        del get_output, tag
        gc.collect()

    def reader():
        """Free capacity so put suspends once and then resumes normally."""
        sc.wait(1, sc.SC_NS)
        assert int(fifo.get().to_uint64()) == 11

    writer_handle = sc.sc_spawn(writer, "f6_writer")
    reader_handle = sc.sc_spawn(reader, "f6_reader")
    sc.sc_start(2, sc.SC_NS)
    assert observations == [
        ("put", 23, "1 ns"),
        ("get_value", 23, "1 ns"),
        ("second_put", 31, "1 ns"),
        ("peek_value", 31, "1 ns"),
        ("peek_output", 31, "1 ns"),
        ("get_output", 31, "1 ns"),
    ], observations
    assert writer_handle.terminated() and reader_handle.terminated()
    gc.collect()
    for key in (
        "input",
        "second_input",
        "get_value",
        "peek_value",
        "peek_output",
        "get_output",
        "tag",
    ):
        assert references[key]() is None, key
    sc.sc_stop()
    assert sc.release_interface_callbacks() == 1
    gc.collect()
    assert error_owner_ref() is None
    try:
        error_interface.get(make_value(79))
    except RuntimeError as error:
        assert "released" in str(error).lower()
    else:
        raise AssertionError("released TLM callback remained callable")
    print("F6_FIFO_FRAME_OK get=peek=put=value=output=tag=error=release")


def quantumkeeper_owner_lifetime():
    """Retain a custom quantum callback through wait/reset and release on deletion."""
    records = []

    class QuantumSource:
        """Provide a copied native time value and one recoverable callback error."""

        def __init__(self):
            """Create stable callback state and its mutable result slot."""
            self.fail = False
            self.slot = sc.sc_time(10, sc.SC_NS)

        def compute(self):
            """Return live time storage or raise the configured hook error."""
            records.append(str(sc.sc_time_stamp()))
            if self.fail:
                raise ValueError("configured quantum hook failure")
            return self.slot

    source = QuantumSource()
    source_ref = weakref.ref(source)
    keeper = tlm_utils.callback_quantumkeeper(source.compute)
    keeper.set_global_quantum(sc.sc_time(10, sc.SC_NS))
    source.fail = True
    try:
        keeper.reset()
    except ValueError as error:
        assert "configured quantum hook failure" in str(error)
    else:
        raise AssertionError("quantum hook error was lost")
    source.fail = False
    keeper.reset()
    source.slot.__assign__(sc.sc_time(2, sc.SC_NS))
    keeper.set(sc.sc_time(9, sc.SC_NS))
    assert not keeper.need_sync(), "quantumkeeper did not copy callback time into native storage"
    source.slot.__assign__(sc.sc_time(10, sc.SC_NS))
    assert source_ref() is source

    def worker():
        """Wait in the native quantum path while the keeper remains borrowed."""
        keeper.set(sc.sc_time(100, sc.SC_NS))
        try:
            keeper.sync()
        finally:
            records.append(f"cleanup:{sc.sc_time_stamp()}")
        raise AssertionError("reset quantum synchronization returned")

    handle = sc.sc_spawn(worker, "f6_quantum_worker")

    def controller():
        """Reset and kill the suspended quantum synchronization process."""
        sc.wait(2, sc.SC_NS)
        handle.reset()
        sc.wait(1, sc.SC_NS)
        handle.kill()

    control = sc.sc_spawn(controller, "f6_quantum_controller")
    sc.sc_start(4, sc.SC_NS)
    assert [value for value in records if value.startswith("cleanup:")] == ["cleanup:2 ns", "cleanup:3 ns"]
    assert handle.terminated() and control.terminated()
    assert source_ref() is source
    sc.sc_stop()

    keeper = None
    source = None
    gc.collect()
    assert source_ref() is None
    print("F6_QUANTUM_OWNER_OK reset=2ns kill=3ns callback=released")


def peq_owner_payload_lifetime():
    """Prove PEQ registry retention, copied phase/time and borrowed payload release."""
    name = sc.sc_module_name("f6_peq_top")
    top = sc.sc_module(name)
    assert top.name() == "f6_peq_top"
    records = []

    class Receiver:
        """Observe borrowed payload identity and copied native delivery metadata."""

        def deliver(self, payload, phase):
            """Record original payload, phase and delivery timestamp."""
            records.append(
                (
                    cppyy.addressof(payload),
                    int(payload.get_address()),
                    str(phase),
                    str(sc.sc_time_stamp()),
                )
            )
            payload.set_response_status(tlm.TLM_OK_RESPONSE)

    receiver = Receiver()
    receiver_ref = weakref.ref(receiver)
    callback = receiver.deliver
    callback_ref = weakref.ref(callback)
    queue = tlm_utils.callback_peq("f6_peq", callback)
    queue_ref = weakref.ref(queue)

    delivered = tlm.tlm_generic_payload()
    delivered.set_address(41)
    delivered_address = cppyy.addressof(delivered)
    delivered_ref = weakref.ref(delivered)
    phase = tlm.tlm_phase(tlm.BEGIN_REQ)
    delay = sc.sc_time(2, sc.SC_NS)
    queue.notify(delivered, phase, delay)

    canceled = tlm.tlm_generic_payload()
    canceled.set_address(43)
    canceled_ref = weakref.ref(canceled)
    queue.notify(canceled, phase, sc.sc_time(5, sc.SC_NS))
    del queue, callback, receiver
    gc.collect()
    assert queue_ref() is not None and callback_ref() is not None and receiver_ref() is not None

    phase.__assign__(tlm.END_RESP)
    delay.__assign__(sc.sc_time(99, sc.SC_NS))
    sc.sc_start(3, sc.SC_NS)
    assert records == [(delivered_address, 41, "BEGIN_REQ", "2 ns")], records
    assert delivered.is_response_ok()
    del delivered
    gc.collect()
    assert delivered_ref() is None

    queue_ref().cancel_all()
    del canceled
    gc.collect()
    assert canceled_ref() is None
    sc.sc_start(3, sc.SC_NS)
    assert len(records) == 1, records
    assert queue_ref() is not None and callback_ref() is not None and receiver_ref() is not None
    sc.sc_stop()
    print("F6_PEQ_OWNER_OK callback=retained payload=borrowed phase=copy delay=copy cancel=released")


SCENARIOS = {
    "structural": structural_owner_lifetime,
    "fifo-frame": fifo_call_frame_lifetime,
    "quantum": quantumkeeper_owner_lifetime,
    "peq": peq_owner_payload_lifetime,
}

if len(sys.argv) != 2 or sys.argv[1] not in SCENARIOS:
    raise SystemExit(f"usage: {sys.argv[0]} {{{', '.join(SCENARIOS)}}}")
SCENARIOS[sys.argv[1]]()
