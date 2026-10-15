"""Verify aligned endian buffers reverse words per bus and restore read data."""

from amal.eda.systemc import tlm
import cppyy

for command in (tlm.TLM_WRITE_COMMAND, tlm.TLM_READ_COMMAND):
    payload = tlm.tlm_generic_payload()
    payload.resize_extensions()
    data = cppyy.gbl.std.vector["unsigned char"](8, 0)
    for index in range(8):
        data[index] = index + 1
    payload.set_command(command)
    payload.set_address(16)
    payload.set_data_ptr(data.data())
    payload.set_data_length(8)
    payload.set_streaming_width(8)
    tlm.tlm_to_hostendian_aligned["unsigned short"](payload, 4)
    converted = payload.get_data_ptr()
    converted.reshape((8,))
    expected = [3, 4, 1, 2, 7, 8, 5, 6]
    if command == tlm.TLM_WRITE_COMMAND:
        assert list(converted) == expected
    else:
        for index in range(8):
            converted[index] = index + 1
    tlm.tlm_from_hostendian(payload)
    assert [ord(value) for value in data] == (list(range(1, 9)) if command == tlm.TLM_WRITE_COMMAND else expected), (
        command,
        list(data),
        expected,
    )
    assert payload.get_address() == 16 and payload.get_data_length() == 8
print("ENDIAN_ALIGNED_OK")
