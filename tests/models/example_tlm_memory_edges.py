"""Exercise the Python TLM example memory and DMI edge cases in isolation."""

from array import array
import sys
from pathlib import Path

from amal.eda.systemc import sc_core as sc, tlm
import cppyy

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "examples/tlm/common"))
from dmi_models import Target


def byte_text(values):
    """Render owned bytes using the native probe's uppercase hexadecimal form."""
    return ",".join(f"{ord(value) if isinstance(value, str) else int(value):02X}" for value in values)


top_name = sc.sc_module_name("top")
top = sc.sc_module(top_name)
target = Target("target", 201)
target.memory = array("B", [0] * 16)
target.read_delay, target.write_delay = 3, 7
cases = (
    ("write4", tlm.TLM_WRITE_COMMAND, 0, 4, 4, False, [0x11, 0x22, 0x33, 0x44]),
    ("read4", tlm.TLM_READ_COMMAND, 0, 4, 4, False, [0xA0, 0xA1, 0xA2, 0xA3]),
    ("byteenable_write", tlm.TLM_WRITE_COMMAND, 4, 4, 4, True, [0x51, 0x62, 0x73, 0x84]),
    ("stream_mismatch_read", tlm.TLM_READ_COMMAND, 4, 4, 3, False, [0xB0, 0xB1, 0xB2, 0xB3]),
    ("length1_read", tlm.TLM_READ_COMMAND, 0, 1, 1, False, [0xC0, 0xC1, 0xC2, 0xC3]),
    ("length5_write", tlm.TLM_WRITE_COMMAND, 8, 5, 5, False, [0x91, 0x82, 0x73, 0x64, 0x55, 0xC5, 0xC6, 0xC7]),
    ("length0_read", tlm.TLM_READ_COMMAND, 15, 0, 0, False, [0xD0, 0xD1, 0xD2, 0xD3]),
    ("exact_end", tlm.TLM_READ_COMMAND, 12, 4, 4, False, [0xE0, 0xE1, 0xE2, 0xE3]),
    ("cross_end", tlm.TLM_WRITE_COMMAND, 13, 4, 4, False, [0x31, 0x32, 0x33, 0x34]),
    ("out_of_range", tlm.TLM_READ_COMMAND, 16, 4, 4, False, [0xF0, 0xF1, 0xF2, 0xF3]),
    ("bad_address_byteenable", tlm.TLM_READ_COMMAND, 16, 4, 4, True, [0xE0, 0xE1, 0xE2, 0xE3]),
    ("ignore_first", tlm.TLM_IGNORE_COMMAND, 0, 4, 4, False, [0xA5, 0xA6, 0xA7, 0xA8]),
    ("ignore_second", tlm.TLM_IGNORE_COMMAND, 0, 4, 4, False, [0xB5, 0xB6, 0xB7, 0xB8]),
    ("bad_address_ignore", tlm.TLM_IGNORE_COMMAND, 16, 4, 4, False, [0xC5, 0xC6, 0xC7, 0xC8]),
)

for label, command, address, length, stream, byte_enable, initial in cases:
    payload = tlm.tlm_generic_payload()
    data = cppyy.gbl.std.vector["unsigned char"](8, 0)
    for index, value in enumerate((initial + [0] * 8)[:8]):
        data[index] = value
    enables = cppyy.gbl.std.vector["unsigned char"](2, 0)
    enables[0], enables[1] = 0xFF, 0x00
    payload.set_command(command)
    payload.set_address(address)
    payload.set_data_ptr(data.data())
    payload.set_data_length(length)
    payload.set_streaming_width(stream)
    payload.set_byte_enable_ptr(enables.data() if byte_enable else cppyy.nullptr)
    payload.set_byte_enable_length(2 if byte_enable else 0)
    payload.set_response_status(tlm.TLM_INCOMPLETE_RESPONSE)
    delay = sc.sc_time(2, sc.SC_NS)
    try:
        target.operation(payload, delay)
        outcome = str(payload.get_response_string())
    except Exception as error:  # noqa: BLE001 - the probe records current edge behavior.
        outcome = f"EXC:{type(error).__name__}"
    print(f"PY|{label}|{outcome}|{delay}|{byte_text(data)}|{byte_text(target.memory)}")

target.memory = array("B", [0] * 4096)
target.read_delay, target.write_delay = 3, 7
for address in (0, 4096, 4097):
    payload = tlm.tlm_generic_payload()
    payload.set_address(address)
    result = tlm.tlm_dmi()
    granted = target.dmi(payload, result)
    print(
        f"PYDMI|enabled|{address}|{int(granted)}|{int(result.get_start_address()) if granted else ''}|"
        f"{int(result.get_end_address()) if granted else ''}|{int(bool(result.get_dmi_ptr())) if granted else ''}|"
        f"{result.get_read_latency() if granted else ''}|{result.get_write_latency() if granted else ''}"
    )
target.enabled = False
for address in (0, 4096, 4097):
    payload = tlm.tlm_generic_payload()
    payload.set_address(address)
    result = tlm.tlm_dmi()
    print(f"PYDMI|disabled|{address}|{int(target.dmi(payload, result))}")

assert top is not None and top_name is not None
