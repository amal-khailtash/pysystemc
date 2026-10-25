"""Compare every endian request/response family with independent native buffers."""

from pathlib import Path

from amal.eda.systemc import tlm
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/tlm_endian_probe.h"))
WORDS = (
    ("unsigned char", 1),
    ("unsigned short", 2),
    ("unsigned int", 4),
    ("unsigned long long", 8),
    ("EndianWord16", 16),
)


def values(vector):
    """Copy owned native byte storage into stable integer observations."""
    return [ord(value) for value in vector]


def check(algorithm, word, size, bus, command, masked, dispatcher):
    """Complete one conversion pair while retaining original borrowed buffers."""
    base = 2**63 + 128
    address = base + (size if size * 2 <= bus else 0) if algorithm == "single" else base
    if algorithm in ("generic", "word"):
        address += 1
    length = size if algorithm == "single" else bus * 2
    stream = bus if algorithm == "generic" else length
    data = cppyy.gbl.std.vector["unsigned char"](length, 0)
    for index in range(length):
        data[index] = index + 3
    enable_length = size * 2 if algorithm == "generic" else length
    enables = cppyy.gbl.std.vector["unsigned char"](enable_length, 0)
    for index in range(enable_length):
        enables[index] = 255 if index // size % 2 == 0 and algorithm != "single" else 0
    payload = tlm.tlm_generic_payload()
    payload.resize_extensions()
    payload.set_command(command)
    payload.set_address(address)
    payload.set_data_ptr(data.data())
    payload.set_data_length(length)
    payload.set_streaming_width(stream)
    if masked:
        payload.set_byte_enable_ptr(enables.data())
        payload.set_byte_enable_length(enable_length)
    original_pointer = cppyy.addressof(payload.get_data_ptr())
    getattr(tlm, "tlm_to_hostendian_" + algorithm)[word](payload, bus)
    expanded = int(payload.get_data_length())
    converted = payload.get_data_ptr()
    converted.reshape((expanded,))
    flags = payload.get_byte_enable_ptr()
    if flags:
        flags.reshape((int(payload.get_byte_enable_length()),))
    converted_flags = list(flags) if flags else []
    active = [i for i in range(expanded) if not flags or flags[i % len(converted_flags)] == 255]
    same_pointer = int(cppyy.addressof(converted) == original_pointer)
    if algorithm == "single":
        assert payload.get_address() == address ^ (bus - size)
        assert same_pointer and expanded == size
    if algorithm == "aligned" and size == bus:
        assert same_pointer
    if command == tlm.TLM_READ_COMMAND:
        for index in active:
            converted[index] = (80 + index) % 256
    observed = [int(converted[index]) for index in active]
    metadata = (int(payload.get_address()), expanded, int(payload.get_streaming_width()), same_pointer)
    if dispatcher:
        tlm.tlm_from_hostendian(payload)
    else:
        getattr(tlm, "tlm_from_hostendian_" + algorithm)[word](payload, bus)
    assert values(enables) == [255 if i // size % 2 == 0 and algorithm != "single" else 0 for i in range(enable_length)]
    if command == tlm.TLM_WRITE_COMMAND:
        assert values(data) == list(range(3, length + 3))
    print(
        "ENDIAN_PUBLIC|"
        + "|".join(
            map(
                str,
                (
                    algorithm,
                    size,
                    bus,
                    int(command),
                    int(masked),
                    int(dispatcher),
                    *metadata,
                    ",".join(map(str, converted_flags)),
                    ",".join(map(str, observed)),
                    ",".join(map(str, values(data))),
                ),
            )
        )
    )


def check_reuse():
    """Reuse a native context across 32/64-bit windows without touching old buffers."""
    payload = tlm.tlm_generic_payload()
    payload.resize_extensions()
    previous = []
    context_address = None
    for address in (2**32 - 1, 2**32 + 1, 2**63 + 131, 2**64 - 65):
        data = cppyy.gbl.std.vector["unsigned char"](16, 11)
        enables = cppyy.gbl.std.vector["unsigned char"](2, 0)
        enables[0] = 255
        payload.set_command(tlm.TLM_READ_COMMAND)
        payload.set_address(address)
        payload.set_data_ptr(data.data())
        payload.set_data_length(16)
        payload.set_streaming_width(8)
        payload.set_byte_enable_ptr(enables.data())
        payload.set_byte_enable_length(2)
        tlm.tlm_to_hostendian_generic["unsigned short"](payload, 8)
        context = payload.get_extension[cppyy.gbl.tlm.tlm_endian_context]()
        if context_address is None:
            context_address = cppyy.addressof(context)
        assert cppyy.addressof(context) == context_address
        assert int(context.address) == address
        assert int(context.new_address) == address & ~7
        converted = payload.get_data_ptr()
        expanded = int(payload.get_data_length())
        converted.reshape((expanded,))
        for index in range(expanded):
            converted[index] = (100 + index) % 256
        width = int(payload.get_streaming_width())
        tlm.tlm_from_hostendian(payload)
        expected = []
        for index in range(16):
            local = index % 8
            mapped = ((address + (local // 2) * 2 + 1 - local % 2) ^ 7) - (address & ~7)
            expected.append((100 + mapped + (index // 8) * width) % 256 if index % 2 == 0 else 11)
        assert values(data) == expected
        assert values(enables) == [255, 0]
        assert all(values(old) == old_values for old, old_values in previous)
        previous.append((data, expected))
        print("ENDIAN_REUSE|" + str(address) + "|" + ",".join(map(str, values(data))))


count = 0
for algorithm in ("generic", "word", "aligned", "single"):
    for word, size in WORDS:
        for bus in (4, 8, 32):
            if size > bus:
                continue
            for command in (tlm.TLM_WRITE_COMMAND, tlm.TLM_READ_COMMAND):
                for masked in (False, True):
                    for dispatcher in (False, True):
                        check(algorithm, word, size, bus, command, masked, dispatcher)
                        count += 1
assert count == 384
check_reuse()
print("ENDIAN_PUBLIC_OK", count)
