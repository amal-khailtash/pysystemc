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
for command in (tlm.TLM_WRITE_COMMAND, tlm.TLM_READ_COMMAND):
    payload = tlm.tlm_generic_payload()
    payload.resize_extensions()
    data = cppyy.gbl.std.vector["unsigned char"](8, 99)
    enables = cppyy.gbl.std.vector["unsigned char"](8, 0)
    for index in (0, 1, 4, 5):
        enables[index] = 255
    payload.set_command(command)
    payload.set_data_ptr(data.data())
    payload.set_data_length(8)
    payload.set_streaming_width(8)
    payload.set_byte_enable_ptr(enables.data())
    payload.set_byte_enable_length(8)
    tlm.tlm_to_hostendian_aligned["unsigned short"](payload, 4)
    converted_enables = payload.get_byte_enable_ptr()
    converted_enables.reshape((8,))
    assert list(converted_enables) == [0, 0, 255, 255, 0, 0, 255, 255]
    converted = payload.get_data_ptr()
    converted.reshape((8,))
    if command == tlm.TLM_READ_COMMAND:
        for index in range(8):
            converted[index] = index + 1
    else:
        assert list(converted) == [99] * 8
    tlm.tlm_from_hostendian(payload)
    expected = [3, 4, 99, 99, 7, 8, 99, 99] if command == tlm.TLM_READ_COMMAND else [99] * 8
    assert [ord(value) for value in data] == expected
    assert [ord(value) for value in enables] == [255, 255, 0, 0, 255, 255, 0, 0]
    assert payload.get_byte_enable_length() == 8
for command in (tlm.TLM_WRITE_COMMAND, tlm.TLM_READ_COMMAND):
    payload = tlm.tlm_generic_payload()
    payload.resize_extensions()
    data = cppyy.gbl.std.vector["unsigned char"](8, 17)
    enables = cppyy.gbl.std.vector["unsigned char"](8, 255)
    payload.set_command(command)
    payload.set_address(32)
    payload.set_data_ptr(data.data())
    payload.set_data_length(8)
    payload.set_streaming_width(8)
    payload.set_byte_enable_ptr(enables.data())
    payload.set_byte_enable_length(8)
    original_data = cppyy.addressof(payload.get_data_ptr())
    original_enables = cppyy.addressof(payload.get_byte_enable_ptr())
    tlm.tlm_to_hostendian_aligned["unsigned int"](payload, 4)
    assert cppyy.addressof(payload.get_data_ptr()) == original_data
    assert cppyy.addressof(payload.get_byte_enable_ptr()) == original_enables
    assert payload.get_address() == 32
    if command == tlm.TLM_READ_COMMAND:
        payload.get_data_ptr()[0] = 23
    tlm.tlm_from_hostendian(payload)
    assert [ord(value) for value in data] == ([23] + [17] * 7 if command == tlm.TLM_READ_COMMAND else [17] * 8)
    assert cppyy.addressof(payload.get_data_ptr()) == original_data
payload = tlm.tlm_generic_payload()
payload.resize_extensions()
data = cppyy.gbl.std.vector["unsigned char"](8, 99)
payload.set_command(tlm.TLM_READ_COMMAND)
payload.set_address(16)
payload.set_data_ptr(data.data())
payload.set_data_length(8)
payload.set_streaming_width(8)
tlm.tlm_to_hostendian_aligned["unsigned short"](payload, 4)
converted = payload.get_data_ptr()
converted.reshape((8,))
for index in range(8):
    converted[index] = index + 1
tlm.tlm_from_hostendian_aligned["unsigned short"](payload, 4)
assert [ord(value) for value in data] == [3, 4, 1, 2, 7, 8, 5, 6]
print("ENDIAN_ALIGNED_OK")
