"""Exercise reachable wait validation and adapter cache/type-selection paths."""

from pathlib import Path

from amal.eda.systemc import sc_core, sc_dt, tlm
import cppyy


ENDIAN_PROBE = Path(__file__).resolve().parents[1] / "native" / "tlm_endian_probe.h"
cppyy.include(str(ENDIAN_PROBE))
cppyy.cppdef(
    r"""
    #include <cstdint>
    #include <vector>

    namespace completion_reachable_edges
    {
    /** @brief Assign a native vector without the Python assignment adapter.
     * @param target Borrowed destination; its owned storage remains live.
     * @param source Borrowed source; it is not retained after this call.
     * @details The call holds the GIL, registers no callbacks, performs no wait,
     * and propagates native conversion/report exceptions unchanged.
     */
    void assign_vector_reference(sc_dt::sc_bv<8>& target, const sc_dt::sc_bv<8>& source)
    {
      target = source;
    }

    /** @brief Call the upstream generic endian conversion as an independent reference.
     * @param payload Borrowed payload; its borrowed data buffers remain live through response.
     * @param bus Native bus width in bytes.
     * @details The call holds the GIL, registers no callbacks, performs no wait,
     * and propagates native conversion/report exceptions unchanged.
     */
    void endian_reference(tlm::tlm_generic_payload* payload, unsigned int bus)
    {
      tlm::tlm_to_hostendian_generic<EndianWord16>(payload, bus);
    }

    /** @brief Dispatch a reference payload through the native response converter.
     * @param payload Borrowed payload whose original data and enable buffers remain live.
     * @details The call holds the GIL, registers no callbacks, performs no wait,
     * and propagates native conversion/report exceptions unchanged.
     */
    void endian_response_reference(tlm::tlm_generic_payload* payload)
    {
      tlm::tlm_from_hostendian(payload);
    }

    /** @brief Return the payload's current data buffer address for ownership checks.
     * @param payload Borrowed payload; no storage is retained.
     * @return The address of its current data buffer, or zero when it is null.
     * @details This synchronous call holds the GIL, registers no callbacks,
     * performs no wait, and propagates native exceptions unchanged.
     */
    std::uintptr_t data_address(const tlm::tlm_generic_payload* payload)
    {
      return reinterpret_cast<std::uintptr_t>(payload->get_data_ptr());
    }

    /** @brief Return the address of an independently owned byte vector.
     * @param bytes Borrowed vector whose storage remains live through the call.
     * @return Its current storage address.
     * @details This synchronous call holds the GIL, registers no callbacks,
     * performs no wait, and propagates native exceptions unchanged.
     */
    std::uintptr_t buffer_address(const std::vector<unsigned char>& bytes)
    {
      return reinterpret_cast<std::uintptr_t>(bytes.data());
    }
    }  // namespace completion_reachable_edges
    """
)


def binary(value):
    """Render a live native bit vector with the native binary prefix."""
    return str(value.to_string(sc_dt.SC_BIN, True))


def bytes_of(vector):
    """Copy an unsigned-char vector into stable Python integer values."""
    return tuple(ord(value) if isinstance(value, str) else int(value) for value in vector)


def emit(*fields):
    """Print one deterministic record for parity with the native concat fixture."""
    print("DAE\t" + "\t".join(map(str, fields)))


def check_warm_text_concat():
    """Compare cold/warm vector-text concatenations with the native fixture."""
    for order in ("head", "tail"):
        owner = sc_dt.sc_bv[8]("01011010")
        for label, value in (("initial", "01011010"), ("updated", "10101010")):
            owner.__assign__(value)
            result = sc_dt.concat("10", owner) if order == "head" else sc_dt.concat(owner, "10")
            expected = "10" + value if order == "head" else value + "10"
            assert result.length() == 10
            assert str(result.to_string()) == expected
            if label == "initial":
                emit("CONCAT", "vector_" + order, label, binary(result))
            else:
                emit("CONCAT", "vector_" + order, label, binary(result), binary(owner))


def check_warm_vector_assignment():
    """Check repeated same-type vector assignments and returned target identity."""
    native = cppyy.gbl.completion_reachable_edges
    vector_type = sc_dt.sc_bv[8]
    source = vector_type("00000000")
    target = vector_type("00000000")
    reference = vector_type("00000000")

    for value in ("10100101", "11000011"):
        source.__assign__(value)
        result = target.__assign__(source)
        native.assign_vector_reference(reference, source)
        assert cppyy.addressof(result) == cppyy.addressof(target)
        assert str(target.to_string()) == str(reference.to_string()) == value


def make_endian_payload(address):
    """Create a payload, its owned bytes, and their original native state."""
    payload = tlm.tlm_generic_payload()
    payload.resize_extensions()
    data = cppyy.gbl.std.vector["unsigned char"](64, 0)
    for index in range(64):
        data[index] = index + 3
    payload.set_command(tlm.TLM_WRITE_COMMAND)
    payload.set_address(address)
    payload.set_data_ptr(data.data())
    payload.set_data_length(64)
    payload.set_streaming_width(32)
    original_pointer = int(cppyy.gbl.completion_reachable_edges.buffer_address(data))
    original_bytes = bytes_of(data)
    return payload, data, original_pointer, original_bytes


def endian_snapshot(payload):
    """Return converted payload bytes and native endian-context metadata."""
    length = int(payload.get_data_length())
    converted = payload.get_data_ptr()
    converted.reshape((length,))
    enables = payload.get_byte_enable_ptr()
    enables.reshape((length,))
    context = payload.get_extension[cppyy.gbl.tlm.tlm_endian_context]()
    return (
        int(payload.get_address()),
        length,
        int(payload.get_streaming_width()),
        tuple((index, int(converted[index])) for index in range(length) if enables[index] == 255),
        tuple(int(enables[index]) for index in range(length)),
        int(context.address),
        int(context.new_address),
    )


def check_native_type_endian_key():
    """Compare native-type dispatch with upstream conversion and response behavior."""
    actual_payload, actual_data, actual_pointer, original = make_endian_payload(0x103)
    reference_payload, reference_data, reference_pointer, reference_original = make_endian_payload(0x103)
    word_type = cppyy.gbl.EndianWord16
    tlm.tlm_to_hostendian_generic[word_type](actual_payload, 32)
    native = cppyy.gbl.completion_reachable_edges
    native.endian_reference(reference_payload, 32)
    assert endian_snapshot(actual_payload) == endian_snapshot(reference_payload)
    assert actual_pointer != reference_pointer
    assert int(native.data_address(actual_payload)) != actual_pointer
    assert int(native.data_address(reference_payload)) != reference_pointer
    assert int(native.data_address(actual_payload)) != int(native.data_address(reference_payload))
    assert original == reference_original
    assert bytes_of(actual_data) == original
    assert bytes_of(reference_data) == reference_original

    converted_pointer = int(native.data_address(actual_payload))
    reference_converted_pointer = int(native.data_address(reference_payload))
    tlm.tlm_from_hostendian(actual_payload)
    native.endian_response_reference(reference_payload)
    # Native write responses keep the converted payload; original buffers are borrowed.
    assert actual_payload.get_address() == reference_payload.get_address() == 0x100
    assert int(native.data_address(actual_payload)) == converted_pointer
    assert int(native.data_address(reference_payload)) == reference_converted_pointer
    assert endian_snapshot(actual_payload) == endian_snapshot(reference_payload)
    assert bytes_of(actual_data) == original
    assert bytes_of(reference_data) == reference_original


def check_invalid_expression_wait_operand():
    """Require the documented TypeError before an invalid wait reaches native code."""
    first = sc_core.sc_event("completion_wait_first")
    second = sc_core.sc_event("completion_wait_second")
    try:
        sc_core.wait(first | second, object())
    except TypeError as error:
        assert str(error) == "wait requires native time/event arguments or a numeric duration"
    else:
        raise AssertionError("expression wait accepted a non-native trailing operand")


def main():
    """Run each bounded reachable adapter case in this fresh process."""
    check_warm_text_concat()
    check_warm_vector_assignment()
    check_native_type_endian_key()
    check_invalid_expression_wait_operand()
    print("COMPLETION_REACHABLE_EDGES_OK wait=TypeError concat=head-tail vector=warm endian=native-type")


if __name__ == "__main__":
    main()
