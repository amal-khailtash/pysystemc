"""Exercise public trace failures and inspect the recovered VCD waveform."""

from pathlib import Path
import ctypes

from amal.eda.systemc import sc_core as sc
import cppyy


class PortModel(sc.sc_module):
    """Own an input port bound to a caller-owned native signal."""

    def __init__(self, name, signal):
        """Construct and bind the module input.

        Args:
            name: Native SystemC module name.
            signal: Signal backing the input port.
        """
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.port = sc.sc_in[int]("port")
        self.port.bind(signal)


def _trace_values(text, signal_name):
    """Return timestamped integer values for one declared VCD signal.

    Args:
        text: Complete VCD file contents.
        signal_name: Public VCD reference name to inspect.

    Returns:
        Ordered (timestamp, value) pairs for the selected signal.

    Raises:
        AssertionError: If the signal has no declaration or usable transitions.
    """
    declaration = next(
        (row.split() for row in text.splitlines() if row.startswith("$var") and row.split()[4] == signal_name),
        None,
    )
    assert declaration is not None, f"missing VCD declaration for {signal_name}"
    identifier = declaration[3]
    timestamp = 0
    values = []
    for line in text.splitlines():
        if line.startswith("#"):
            timestamp = int(line[1:])
        elif line.startswith("b"):
            value, actual_identifier = line.split()
            if actual_identifier == identifier:
                values.append((timestamp, int(value[1:], 2)))
    assert values, f"no VCD transitions for {signal_name}"
    return values


signal = sc.sc_signal[int]("recovery_signal", 3)
model = PortModel("recovery_model", signal)
port = model.port
char_storage = (ctypes.c_char * 1)(b"Q")
char_pointer = ctypes.cast(char_storage, ctypes.POINTER(ctypes.c_char))
char_signature = "sc_core::sc_trace_file*,const char*,const std::string&,int"
selected_char_trace = sc.sc_trace.__overload__(char_signature)
native_char_trace = cppyy.gbl.sc_core.sc_trace.__overload__(char_signature)

# A real cppyy-selected native overload accepts this positional call and safely
# ignores it because the trace-file pointer is null.
native_char_trace(cppyy.nullptr, "X", "native_null_probe", 8)

with sc.vcd_trace("trace_error_recovery", time_unit=(1, sc.SC_NS)) as trace:
    try:
        sc.tprintf(trace, "%5%")
    except ValueError as error:
        assert "literal percent" in str(error)
    else:
        raise AssertionError("decorated literal percent was accepted")
    sc.tprintf(trace, "RECOVERED_PRINTF %d", 17)

    count_storage = (ctypes.c_int * 3)(-1, -1, -1)
    try:
        sc.tprintf(trace, "REJECTED_COUNT%n", ctypes.byref(count_storage, 1))
    except TypeError as error:
        assert "aligned count storage" in str(error)
    else:
        raise AssertionError("misaligned in-bounds %n storage was accepted")
    assert list(count_storage) == [-1, -1, -1], list(count_storage)
    sc.tprintf(trace, "RECOVERED_COUNT%n", ctypes.byref(count_storage))
    assert count_storage[0] == len("RECOVERED_COUNT"), count_storage[0]

    try:
        sc.sc_trace(trace, port, "rejected_port_width", 8)
    except TypeError as error:
        assert "port tracing does not accept width" in str(error)
    else:
        raise AssertionError("sc_in tracing accepted an explicit width")
    sc.sc_trace(trace, port, "recovered_port")

    try:
        selected_char_trace(file=trace, value=char_pointer, name="rejected_char_kwargs", width=8)
    except TypeError:
        pass
    else:
        raise AssertionError("selected cppyy overload accepted unsupported keyword arguments")
    selected_char_trace(trace, char_pointer, "recovered_char_pointer", 8)

    sc.sc_start(1, sc.SC_NS)
    signal.write(9)
    char_storage[0] = b"R"
    sc.sc_start(1, sc.SC_NS)

vcd = Path("trace_error_recovery.vcd").read_text()
assert "RECOVERED_PRINTF 17" in vcd, vcd
assert "RECOVERED_COUNT" in vcd and "REJECTED_COUNT" not in vcd, vcd
assert "rejected_port_width" not in vcd and "rejected_char_kwargs" not in vcd, vcd
for name, width in (("recovered_port", 32), ("recovered_char_pointer", 8)):
    declaration = next(row.split() for row in vcd.splitlines() if row.startswith("$var") and row.split()[4] == name)
    assert int(declaration[2]) == width, declaration

port_values = _trace_values(vcd, "recovered_port")
char_values = _trace_values(vcd, "recovered_char_pointer")
assert (0, 3) in port_values and (1, 9) in port_values, port_values
assert (0, ord("Q")) in char_values and (1, ord("R")) in char_values, char_values
assert "$dumpvars" in vcd and "#1" in vcd and "#2" in vcd, vcd
print("TRACE_ERROR_RECOVERY_OK")
