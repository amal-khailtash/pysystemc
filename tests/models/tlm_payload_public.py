"""Exercise generic payload fields, borrowed buffers and extension ownership."""

import gc
from pathlib import Path

from amal.eda.systemc import tlm
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/tlm_payload_probe.h"))
probe = cppyy.gbl.payload_probe
Extension = probe.Extension[137]


def address(value):
    """Return the address of a borrowed native object or zero for null."""
    return cppyy.addressof(value) if value else 0


def bytes_at(pointer, count):
    """Read caller-owned byte storage through its borrowed native pointer."""
    pointer.reshape((count,))
    return [int(pointer[index]) for index in range(count)]


empty = tlm.tlm_generic_payload()
assert not empty.has_mm() and empty.get_ref_count() == 0
assert empty.get_address() == empty.get_data_length() == empty.get_byte_enable_length() == 0
assert not empty.get_data_ptr() and not empty.get_byte_enable_ptr()
assert empty.get_command() == tlm.TLM_IGNORE_COMMAND
assert not empty.is_read() and not empty.is_write()
assert not empty.is_dmi_allowed() and empty.get_streaming_width() == 0
assert empty.get_response_status() == tlm.TLM_INCOMPLETE_RESPONSE
assert empty.is_response_error() and not empty.is_response_ok()
assert empty.get_gp_option() == tlm.TLM_MIN_PAYLOAD

fixture = probe.Fixture()
source, copy = fixture.source, fixture.copy
source.set_data_ptr(fixture.source_data)
source.set_byte_enable_ptr(fixture.source_enable)
copy.set_data_ptr(fixture.copy_data)
copy.set_byte_enable_ptr(fixture.copy_enable)
source.set_address(0xFEDCBA9876543210)
source.set_data_length(16)
source.set_byte_enable_length(8)
source.set_streaming_width(7)
source.set_write()
assert source.is_write() and not source.is_read()
source.set_command(tlm.TLM_IGNORE_COMMAND)
assert not source.is_write() and not source.is_read()
source.set_read()
assert source.is_read() and source.get_command() == tlm.TLM_READ_COMMAND
source.set_dmi_allowed(True)
source.set_gp_option(tlm.TLM_FULL_PAYLOAD)
assert bytes_at(source.get_data_ptr(), 16) == list(range(1, 17))
assert bytes_at(source.get_byte_enable_ptr(), 8) == [255, 0] * 4

# cppyy rejects negative unsigned inputs before entering the native setter.
for invalid in (-1, 1 << 80):
    try:
        source.set_address(invalid)
    except ValueError:
        pass
    else:
        raise AssertionError("invalid uint64 conversion was accepted")
    assert source.get_address() == 0xFEDCBA9876543210
for name, setter, getter, baseline in (
    ("length", source.set_data_length, source.get_data_length, 16),
    ("enable", source.set_byte_enable_length, source.get_byte_enable_length, 8),
    ("stream", source.set_streaming_width, source.get_streaming_width, 7),
):
    try:
        setter(-1)
    except ValueError:
        pass
    else:
        raise AssertionError("negative unsigned conversion was accepted")
    assert getter() == baseline
    setter(1 << 40)
    assert getter() == 0  # Native unsigned-int conversion preserves its low 32 bits.
    print(f"PAYLOAD_UINT32|{name}|{getter()}")
    setter(baseline)
del setter, getter

for status, spelling in (
    (tlm.TLM_OK_RESPONSE, "TLM_OK_RESPONSE"),
    (tlm.TLM_INCOMPLETE_RESPONSE, "TLM_INCOMPLETE_RESPONSE"),
    (tlm.TLM_GENERIC_ERROR_RESPONSE, "TLM_GENERIC_ERROR_RESPONSE"),
    (tlm.TLM_ADDRESS_ERROR_RESPONSE, "TLM_ADDRESS_ERROR_RESPONSE"),
    (tlm.TLM_COMMAND_ERROR_RESPONSE, "TLM_COMMAND_ERROR_RESPONSE"),
    (tlm.TLM_BURST_ERROR_RESPONSE, "TLM_BURST_ERROR_RESPONSE"),
    (tlm.TLM_BYTE_ENABLE_ERROR_RESPONSE, "TLM_BYTE_ENABLE_ERROR_RESPONSE"),
):
    source.set_response_status(status)
    assert source.get_response_status() == status
    assert source.get_response_string() == spelling
    assert source.is_response_ok() == (status == tlm.TLM_OK_RESPONSE)
    assert source.is_response_error() == (status != tlm.TLM_OK_RESPONSE)
    print(f"PAYLOAD_STATUS|{int(status)}|{spelling}")
source.set_response_status(tlm.TLM_OK_RESPONSE)

extension = Extension(7)
extension.__python_owns__ = False
source.resize_extensions()
assert not source.set_extension[Extension](extension)
extension_id = int(Extension.ID)
assert address(source.get_extension(extension_id)) == address(extension)
assert address(source.get_extension[Extension]()) == address(extension)
output = cppyy.bind_object(0, Extension)
source.get_extension[Extension](output)
assert address(output) == address(extension)
copy.deep_copy_from(source)
cloned = copy.get_extension[Extension]()
assert address(cloned) != address(extension)
assert cloned.value[136].to_bool() and cloned.value.to_uint64() == 7
assert int(Extension.clones) == 1
assert copy.get_address() == source.get_address()
assert copy.get_data_length() == 16 and copy.get_byte_enable_length() == 8
assert copy.get_streaming_width() == 7 and copy.get_command() == tlm.TLM_READ_COMMAND
assert copy.is_dmi_allowed() and copy.get_gp_option() == tlm.TLM_FULL_PAYLOAD
assert bytes_at(copy.get_data_ptr(), 16) == list(range(1, 17))
assert bytes_at(copy.get_byte_enable_ptr(), 8) == [255, 0] * 4
fixture.copy_data[0] = 77
assert int(fixture.source_data[0]) == 1
copy.deep_copy_from(source)
assert int(Extension.clones) == 1 and int(Extension.copies) == 1
cloned.value = probe.Extension[137](9).value
source.update_extensions_from(copy)
assert extension.value.to_uint64() == 9 and extension.value[136].to_bool()
assert int(Extension.copies) == 2

for enable_length in (8, 4, 3):
    source.set_byte_enable_length(enable_length)
    for index in range(16):
        fixture.source_data[index] = index + 1
        fixture.copy_data[index] = 100 + index
    copy.set_response_status(tlm.TLM_ADDRESS_ERROR_RESPONSE)
    copy.set_dmi_allowed(False)
    source.update_original_from(copy)
    expected = [100 + i if (i % enable_length) % 2 == 0 else i + 1 for i in range(16)]
    assert bytes_at(source.get_data_ptr(), 16) == expected
    assert source.get_response_status() == tlm.TLM_ADDRESS_ERROR_RESPONSE and not source.is_dmi_allowed()
    print("PAYLOAD_MASK|" + str(enable_length) + "|" + ",".join(map(str, expected)))
    source.update_original_from(copy, False)
    assert bytes_at(source.get_data_ptr(), 16) == list(range(100, 116))
source.set_write()
fixture.source_data[0] = 88
source.update_original_from(copy, False)
assert int(fixture.source_data[0]) == 88

source.clear_extension(extension)
assert not source.get_extension[Extension]()
assert extension.value.to_uint64() == 9
assert not source.set_extension(extension_id, extension)
source.clear_extension[Extension]()
assert not source.get_extension[Extension]()
extension.free()
copy.free_all_extensions()
assert not copy.get_extension[Extension]()
assert int(Extension.live) == 0 and int(Extension.frees) == 2

for pointer_argument in (True, False):
    value = Extension(31)
    value.__python_owns__ = False
    source.set_extension(value)
    if pointer_argument:
        source.release_extension(value)
    else:
        source.release_extension[Extension]()
    assert not source.get_extension[Extension]()
assert int(Extension.live) == 0 and int(Extension.frees) == 4

previous, replacement = Extension(51), Extension(52)
previous.__python_owns__ = replacement.__python_owns__ = False
assert not source.set_extension(previous)
assert address(source.set_extension[Extension](replacement)) == address(previous)
previous.free()
source.release_extension[Extension]()
assert int(Extension.live) == 0 and int(Extension.frees) == 6

source.set_mm(fixture.manager)
assert source.has_mm()
managed = tlm.tlm_generic_payload(fixture.manager)
assert managed.has_mm() and managed.get_ref_count() == 0
for indexed in (False, True):
    value = Extension(41)
    value.__python_owns__ = False
    if indexed:
        assert not source.set_auto_extension(extension_id, value)
    else:
        assert not source.set_auto_extension[Extension](value)
    source.set_gp_option(tlm.TLM_FULL_PAYLOAD_ACCEPTED)
    source.acquire()
    source.acquire()
    assert source.get_ref_count() == 2
    source.release()
    assert source.get_ref_count() == 1
    source.release()
    assert source.get_ref_count() == 0
    assert address(fixture.manager.last) == address(source)
    assert source.get_gp_option() == tlm.TLM_MIN_PAYLOAD
    assert not source.get_extension[Extension]()
assert fixture.manager.releases == 2 and int(Extension.frees) == 8
for pointer_argument in (True, False):
    value = Extension(61)
    value.__python_owns__ = False
    source.set_extension(value)
    if pointer_argument:
        source.release_extension(value)
    else:
        source.release_extension[Extension]()
    assert address(source.get_extension[Extension]()) == address(value)
    assert int(Extension.live) == 1
    source.reset()
    assert not source.get_extension[Extension]() and int(Extension.live) == 0
assert int(Extension.frees) == 10
source.set_mm(cppyy.nullptr)
assert not source.has_mm()
source.set_gp_option(tlm.TLM_FULL_PAYLOAD)
source.reset()
assert source.get_gp_option() == tlm.TLM_MIN_PAYLOAD
assert source.get_address() == 0xFEDCBA9876543210 and source.get_data_length() == 16

context = tlm.tlm_endian_context()
context.__python_owns__ = False
assert not source.set_extension[tlm.tlm_endian_context](context)
assert address(source.get_extension[tlm.tlm_endian_context]()) == address(context)
source.clear_extension(context)
context.free()
source.set_data_ptr(cppyy.nullptr)
source.set_byte_enable_ptr(cppyy.nullptr)
assert not source.get_data_ptr() and not source.get_byte_enable_ptr()
empty.deep_copy_from(source)
assert empty.get_address() == source.get_address() and empty.get_data_length() == 16
assert not empty.get_data_ptr() and not empty.get_byte_enable_ptr()
empty.update_original_from(copy)
assert empty.get_response_status() == copy.get_response_status()
last = Extension(71)
last.__python_owns__ = False
empty.set_extension(last)
assert int(Extension.live) == 1
del cloned, output, extension, previous, replacement, source, copy, fixture, empty, managed, last
gc.collect()
assert int(Extension.live) == 0 and int(Extension.frees) == 11
print("PAYLOAD_LIFETIME|1|11|2")
print("TLM_PAYLOAD_PUBLIC_OK")
