"""Compare native TLM enums, version metadata, endianness and registry state."""

from pathlib import Path
import sys

from amal.eda.systemc import tlm
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/tlm_namespace_probe.h"))


def text(value):
    """Encode native scalar/string values as stable single-line records."""
    if isinstance(value, int):
        return str(int(value))
    return str(value).replace("\\", "\\\\").replace("\n", "\\n").replace("\r", "\\r")


SCALARS = [
    "BEGIN_REQ",
    "BEGIN_RESP",
    "END_REQ",
    "END_RESP",
    "TLM_ACCEPTED",
    "TLM_ADDRESS_ERROR_RESPONSE",
    "TLM_BIG_ENDIAN",
    "TLM_BURST_ERROR_RESPONSE",
    "TLM_BYTE_ENABLE_ERROR_RESPONSE",
    "TLM_COMMAND_ERROR_RESPONSE",
    "TLM_COMPLETED",
    "TLM_FULL_PAYLOAD",
    "TLM_FULL_PAYLOAD_ACCEPTED",
    "TLM_GENERIC_ERROR_RESPONSE",
    "TLM_IGNORE_COMMAND",
    "TLM_INCOMPLETE_RESPONSE",
    "TLM_INITIATOR_SOCKET",
    "TLM_LITTLE_ENDIAN",
    "TLM_MIN_PAYLOAD",
    "TLM_MULTI_INITIATOR_SOCKET",
    "TLM_MULTI_SOCKET",
    "TLM_MULTI_TARGET_SOCKET",
    "TLM_OK_RESPONSE",
    "TLM_READ_COMMAND",
    "TLM_TARGET_SOCKET",
    "TLM_UNKNOWN_ENDIAN",
    "TLM_UNKNOWN_SOCKET",
    "TLM_UPDATED",
    "TLM_WRITE_COMMAND",
    "UNINITIALIZED_PHASE",
    "tlm_copyright_string",
    "tlm_is_prerelease",
    "tlm_version_major",
    "tlm_version_minor",
    "tlm_version_originator",
    "tlm_version_patch",
    "tlm_version_prerelease",
    "tlm_version_release_date",
    "tlm_version_string",
    "tlm_version_string_2",
]
for name in SCALARS:
    print("TLM_NAMESPACE|" + name + "|" + text(getattr(tlm, name)))
ENUMS = {
    "tlm_command": "TLM_IGNORE_COMMAND",
    "tlm_endianness": "TLM_BIG_ENDIAN",
    "tlm_gp_option": "TLM_FULL_PAYLOAD",
    "tlm_phase_enum": "BEGIN_REQ",
    "tlm_response_status": "TLM_ADDRESS_ERROR_RESPONSE",
    "tlm_socket_category": "TLM_INITIATOR_SOCKET",
    "tlm_sync_enum": "TLM_ACCEPTED",
}
for name, representative in ENUMS.items():
    enum_type = getattr(tlm, name)
    value = getattr(tlm, representative)
    assert isinstance(value, enum_type)
    assert int(enum_type(int(value))) == int(value)
    print(f"TLM_ENUM|{name}|{int(value)}|1")
FUNCTIONS = ["get_host_endianness", "host_has_little_endianness", "tlm_copyright", "tlm_release", "tlm_version"]
for name in FUNCTIONS:
    print("TLM_FUNCTION|" + name + "|" + text(getattr(tlm, name)()))
for value in (tlm.TLM_UNKNOWN_ENDIAN, tlm.TLM_LITTLE_ENDIAN, tlm.TLM_BIG_ENDIAN):
    print(f"TLM_HOST|{int(value)}|{int(tlm.has_host_endianness(value))}")
expected = tlm.TLM_LITTLE_ENDIAN if sys.byteorder == "little" else tlm.TLM_BIG_ENDIAN
assert tlm.get_host_endianness() == expected
assert tlm.host_has_little_endianness() == (sys.byteorder == "little")
assert tlm.has_host_endianness(expected) and not tlm.has_host_endianness(tlm.TLM_UNKNOWN_ENDIAN)
assert str(tlm.tlm_release()) == str(tlm.tlm_version_string)
assert str(tlm.tlm_version()) == str(tlm.tlm_version_string_2)
assert str(tlm.tlm_copyright()) == str(tlm.tlm_copyright_string)
probe = cppyy.gbl.tlm_namespace_probe
before = int(tlm.max_num_extensions())
identifier = int(probe.Registry.register_type())
after = int(tlm.max_num_extensions())
assert identifier == before and after == before + 1
assert int(probe.Registry.register_type()) == identifier and int(tlm.max_num_extensions()) == after
print("TLM_REGISTRY|1|1|1")
array = tlm.tlm_array["tlm::tlm_extension_base*"](2)
assert array.size() == 2 and array.kind() == "tlm_array"
array.expand(5)
assert array.size() == 5 and probe.null_slots(array) == 5
array.expand(3)
assert array.size() == 5
print("TLM_ARRAY|tlm_array|5|5")
