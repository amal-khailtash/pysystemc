"""Compare Python stream adapters for public SystemC insertion overloads."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy
import pathlib

cppyy.include(str(pathlib.Path(__file__).resolve().parents[1] / "native" / "sc_core_public_streams_probe.h"))
probe = cppyy.gbl.sc_core_public_streams_probe

WIDE_TEXT = "87112285931760246646623899502532662132737"
wide_type = sc_dt.sc_biguint[137]
payload_type = probe.Payload
wide_value = wide_type(WIDE_TEXT)

bool_signal = sc.sc_signal[bool]("bool_signal", True)
bool_inout_signal = sc.sc_signal[bool]("bool_inout_signal", False)
bool_output_signal = sc.sc_signal[bool]("bool_output_signal", True)
wide_signal = sc.sc_signal[wide_type]("wide_signal", wide_value)
payload_buffer = sc.sc_buffer[payload_type]("payload_buffer", payload_type(23))

ports = probe.Ports("stream_ports")
ports.bool_input.bind(bool_signal)
ports.bool_inout.bind(bool_inout_signal)
ports.bool_output.bind(bool_output_signal)
ports.wide_input.bind(wide_signal)
ports.wide_inout.bind(wide_signal)
ports.payload_input.bind(payload_buffer)
ports.payload_inout.bind(payload_buffer)

sc.sc_start(sc.SC_ZERO_TIME)


def capture(label, stream, *values):
    """Insert values through native adapters and return one flattened record.

    Args:
        label: Native/Python comparison row name.
        stream: Borrowed native character ostream retained through insertion.
        values: Status, time, channel/port, text, or scalar operands to insert.

    Returns:
        The flattened native stream contents copied as a Python string.

    Raises:
        OSError: If a native ios operation reports badbit/failbit.
    """
    result = stream
    for value in values:
        result = result << value
    assert cppyy.addressof(result) == probe.address(stream)
    rendered = str(stream.str()).replace("\n", "|")
    print("STREAM", label, rendered, sep="\t")
    return rendered


capture(
    "status",
    cppyy.gbl.std.ostringstream(),
    "status=",
    sc.ScStatus.SC_RUNNING,
    "|",
    cppyy.gbl.sc_core.sc_status.SC_STOPPED,
    "|",
    41,
)
capture("time", cppyy.gbl.std.stringstream(), "time=", sc.sc_time(12, sc.SC_NS), "|", 41)
capture(
    "channels",
    cppyy.gbl.std.ostringstream(),
    "channels=",
    bool_signal,
    "|",
    wide_signal,
    "|",
    payload_buffer,
    "|",
    41,
)
capture(
    "ports",
    cppyy.gbl.std.ostringstream(),
    "ports=",
    ports.bool_input,
    "|",
    ports.bool_inout,
    "|",
    ports.bool_output,
    "|",
    ports.wide_input,
    "|",
    ports.wide_inout,
    "|",
    ports.payload_input,
    "|",
    ports.payload_inout,
    "|",
    41,
)
capture(
    "derived",
    probe.LateOstream(),
    "derived=",
    cppyy.gbl.sc_core.sc_status.SC_END_OF_INITIALIZATION,
    "|",
    wide_signal,
    "|",
    137,
)

assert bool_signal.read() is True
assert bool_inout_signal.read() is False
assert bool_output_signal.read() is True
assert wide_signal.read().to_string(sc_dt.SC_DEC) == WIDE_TEXT
assert payload_buffer.read().value == 23
assert ports.bool_input.read() is True
assert ports.bool_inout.read() is False
assert ports.bool_output.read() is True
assert ports.wide_input.read().to_string(sc_dt.SC_DEC) == WIDE_TEXT
assert ports.wide_inout.read().to_string(sc_dt.SC_DEC) == WIDE_TEXT
assert ports.payload_input.read().value == 23
assert ports.payload_inout.read().value == 23

failed = cppyy.gbl.std.ostringstream()
try:
    probe.fail_stream(failed)
except Exception:
    pass
else:
    raise AssertionError("badbit did not throw")
try:
    failed << bool_signal
except OSError as error:
    assert "iostream error" in str(error), error
    diagnostic = str(error).rsplit(": ", 1)[-1]
else:
    raise AssertionError("failed stream accepted channel insertion")
assert bool_signal.read() is True and payload_buffer.read().value == 23
print("STREAM", "error", diagnostic, sep="\t")

sc.sc_stop()
