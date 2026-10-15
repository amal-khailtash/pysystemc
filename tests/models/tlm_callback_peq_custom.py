"""Check custom native protocol payloads/phases and PEQ callback error propagation."""

from amal.eda.systemc import sc_core as sc, tlm_utils
import cppyy

cppyy.cppdef("""
/** @brief Arbitrary native payload used to validate protocol template preservation. */
struct PeqPayload { sc_dt::sc_biguint<137> value; };
/** @brief Custom protocol with integral phase rather than tlm_phase. */
struct PeqProtocol { using tlm_payload_type = PeqPayload; using tlm_phase_type = unsigned int; };
""")
name = sc.sc_module_name("top")
top = sc.sc_module(name)
payload = cppyy.gbl.PeqPayload()
payload.value = cppyy.gbl.sc_dt.sc_biguint[137]("0x10000000000000000000000000000000011")
records = []


def deliver(trans, phase):
    """Preserve borrowed payload identity and custom phase before raising a callback error."""
    assert cppyy.addressof(trans) == cppyy.addressof(payload)
    assert trans.value[136] and trans.value.to_uint64() == 17
    records.append((int(phase), str(sc.sc_time_stamp())))
    trans.value = cppyy.gbl.sc_dt.sc_biguint[137](23)
    raise ValueError("custom PEQ callback failure")


queue = tlm_utils.callback_peq("custom_queue", deliver, cppyy.gbl.PeqProtocol)
queue.notify(payload, 73, sc.sc_time(2, sc.SC_NS))
del name
try:
    sc.sc_start(sc.sc_time(3, sc.SC_NS))
except Exception as error:
    assert "custom PEQ callback failure" in str(error), error
else:
    raise AssertionError("callback exception was swallowed")
assert records == [(73, "2 ns")], records
assert payload.value.to_uint64() == 23
assert sc.sc_time_stamp() == sc.sc_time(2, sc.SC_NS)
print("CUSTOM_CALLBACK_PEQ_OK")
