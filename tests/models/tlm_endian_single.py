"""Verify native single-word endian address conversion and response dispatch."""

from amal.eda.systemc import tlm

for word, size in (("unsigned char", 1), ("unsigned short", 2), ("unsigned int", 4)):
    for bus in (4, 8, 16):
        for address in (0, 16, 32):
            payload = tlm.tlm_generic_payload()
            payload.resize_extensions()
            payload.set_address(address)
            payload.set_command(tlm.TLM_READ_COMMAND)
            payload.set_data_length(size)
            payload.set_streaming_width(size)
            tlm.tlm_to_hostendian_single[word](payload, bus)
            expected = address + bus - size
            assert payload.get_address() == expected
            payload.set_response_status(tlm.TLM_OK_RESPONSE)
            tlm.tlm_from_hostendian(payload)
            assert payload.get_address() == expected and payload.is_response_ok()
print("ENDIAN_SINGLE_OK")
