"""Compare public PEQ aliases, borrowed payload delivery and callback timing."""

from pathlib import Path
import gc

from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[2] / "tests/native/tlm_peq_probe.h"))


def make_payload(index):
    """Create caller-owned custom payload with a set 137th data bit."""
    payload = cppyy.gbl.PeqPayload()
    payload.id = index
    payload.value = cppyy.gbl.sc_dt.sc_biguint[137](f"0x1{'0' * 32}{index + 17:02x}")
    return payload


top_name = sc.sc_module_name("top")
top = sc.sc_module(top_name)
native_callback_probe = cppyy.gbl.PeqNativeCallbackProbe()
native_default_probe = cppyy.gbl.PeqNativeDefaultCallbackProbe()
payloads = [make_payload(index) for index in range(6)]
payload_type = cppyy.gbl.PeqPayload
protocol_type = cppyy.gbl.PeqProtocol
get_type = tlm_utils.peq_with_get[payload_type]
get_queue = get_type("get_queue")
cancelled_get_queue = get_type("cancelled_queue")
assert get_type.transaction_type == cppyy.gbl.PeqPayload
pair_type = get_type.pair_type
cppyy.cppdef(
    """
    /** @brief Build the native public pair alias from a time and borrowed payload. */
    inline tlm_utils::peq_with_get<PeqPayload>::pair_type make_peq_pair(
        const sc_core::sc_time& time, PeqPayload& payload) {
      return {time, &payload};
    }
    """
)
sample_pair_time = sc.sc_time(9, sc.SC_NS)
sample_pair = cppyy.gbl.make_peq_pair(sample_pair_time, payloads[0])
assert type(sample_pair) is pair_type
assert sample_pair.first == sample_pair_time
assert cppyy.addressof(sample_pair.second) == cppyy.addressof(payloads[0])
print("PEQ_PAIR_TYPE_OK")
assert cppyy.addressof(get_queue.get_event()) == cppyy.addressof(get_queue.get_event())

get_records = []
callback_records = []


def deliver(payload, phase):
    """Record the borrowed payload pointer, copied phase and native time."""
    assert payload.value[136].to_bool()
    assert payload.value.to_uint64() == payload.id + 17
    callback_records.append((int(payload.id), int(phase), str(sc.sc_time_stamp()), int(sc.sc_delta_count())))


callback_queue = tlm_utils.callback_peq("callback", deliver, protocol_type)
cancelled_callback_queue = tlm_utils.callback_peq("cancelled_callback", deliver, protocol_type)
default_callback_records = []


def deliver_default(payload, phase):
    """Record the default protocol's borrowed payload and copied phase."""
    default_callback_records.append(
        (int(payload.get_address()), str(phase), str(sc.sc_time_stamp()), int(sc.sc_delta_count()))
    )


default_callback_queue = tlm_utils.callback_peq("default_callback", deliver_default)
standard_payload = tlm.tlm_generic_payload()
standard_payload.set_address(601)
standard_phase = tlm.tlm_phase(tlm.BEGIN_REQ)


def produce():
    """Schedule all notification kinds and cancel one item from each queue."""
    global native_callback_probe, native_default_probe

    get_queue.notify(payloads[0])
    get_queue.notify(payloads[1])
    get_queue.notify(payloads[2], sc.SC_ZERO_TIME)
    get_queue.notify(payloads[3], sc.sc_time(2, sc.SC_NS))
    get_queue.notify(payloads[4], sc.sc_time(2, sc.SC_NS))
    cancelled_get_queue.notify(payloads[5], sc.sc_time(5, sc.SC_NS))
    cancelled_get_queue.cancel_all()

    callback_queue.notify(payloads[0], 41)
    callback_queue.notify(payloads[1], 42)
    callback_queue.notify(payloads[2], 43, sc.SC_ZERO_TIME)
    callback_queue.notify(payloads[3], 44, sc.sc_time(2, sc.SC_NS))
    callback_queue.notify(payloads[4], 45, sc.sc_time(2, sc.SC_NS))
    cancelled_callback_queue.notify(payloads[5], 46, sc.sc_time(5, sc.SC_NS))
    cancelled_callback_queue.cancel_all()
    default_callback_queue.notify(standard_payload, standard_phase, sc.sc_time(1, sc.SC_NS))

    native_callback_probe.unnamed_queue.notify(payloads[0], 41)
    native_callback_probe.unnamed_queue.notify(payloads[1], 42)
    native_callback_probe.unnamed_queue.notify(payloads[2], 43, sc.SC_ZERO_TIME)
    native_callback_probe.named_queue.notify(payloads[3], 44, sc.sc_time(2, sc.SC_NS))
    native_callback_probe.named_queue.notify(payloads[4], 45, sc.sc_time(2, sc.SC_NS))
    native_default_probe.unnamed_queue.notify(standard_payload, standard_phase, sc.sc_time(1, sc.SC_NS))
    standard_phase.__assign__(tlm.END_RESP)
    sc.wait(6, sc.SC_NS)
    assert not cancelled_get_queue.get_next_transaction()
    assert [record[0] for record in callback_records] == [0, 1, 2, 3, 4], callback_records
    print(f"PEQ_CANCEL_OK {sc.sc_time_stamp()}")


def consume():
    """Drain queue notifications and assert the original payload addresses."""
    expected = [0, 1, 2, 3, 4]
    count = 0
    stable_event = cppyy.addressof(get_queue.get_event())
    while count < len(expected):
        sc.wait(get_queue.get_event())
        assert cppyy.addressof(get_queue.get_event()) == stable_event
        while True:
            payload = get_queue.get_next_transaction()
            if not payload:
                break
            assert cppyy.addressof(payload) == cppyy.addressof(payloads[expected[count]])
            assert payload.value[136].to_bool()
            assert payload.value.to_uint64() == payload.id + 17
            record = (int(payload.id), str(sc.sc_time_stamp()), int(sc.sc_delta_count()))
            get_records.append(record)
            print(f"PEQ_GET {record[0]} {record[1]} {record[2]}")
            count += 1


handles = [sc.sc_spawn(produce, "produce"), sc.sc_spawn(consume, "consume")]
sc.sc_start(7, sc.SC_NS)
assert all(handle.terminated() for handle in handles)
assert [record[0] for record in get_records] == [0, 1, 2, 3, 4], get_records
assert [record[:2] for record in callback_records] == [
    (0, 41),
    (1, 42),
    (2, 43),
    (3, 44),
    (4, 45),
], callback_records
for record in callback_records:
    print(f"PEQ_ADAPTER_CB {record[0]} {record[1]} {record[2]} {record[3]}")
assert [(record[0], record[1]) for record in default_callback_records] == [(601, "BEGIN_REQ")]
for record in default_callback_records:
    print(f"PEQ_ADAPTER_DEFAULT {record[0]} {record[1]} {record[2]} {record[3]}")

native_callback_records = list(native_callback_probe.unnamed_owner.records) + list(
    native_callback_probe.named_owner.records
)
assert [int(record.id) for record in native_callback_records] == [0, 1, 2, 3, 4], native_callback_records
for record in native_callback_records:
    assert int(record.payload_address) == cppyy.addressof(payloads[int(record.id)])
    assert payloads[int(record.id)].value[136].to_bool()
    assert payloads[int(record.id)].value.to_uint64() == int(record.id) + 17
    print(f"PEQ_CB {record.id} {record.phase} {record.time} {record.delta}")

native_default_records = list(native_default_probe.unnamed_owner.records)
assert len(native_default_records) == 1
default_record = native_default_records[0]
assert default_record.address == 601 and default_record.begin_request
assert int(default_record.payload_address) == cppyy.addressof(standard_payload)
print(f"PEQ_DEFAULT {default_record.address} BEGIN_REQ {default_record.time} {default_record.delta}")
print("PEQ_PYTHON_OK")
sc.sc_stop()
del native_callback_probe, native_default_probe
gc.collect()
