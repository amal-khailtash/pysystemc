"""Check public datatype streams recover after native ios exceptions."""

from amal.eda.systemc import sc_dt as dt
import cppyy


STD = cppyy.gbl.std
INITIAL = (1 << 136) | 123456789
RECOVERED = INITIAL - 4


def decimal(value):
    """Render an arbitrary-width native integer without narrowing."""
    return str(value.to_string(dt.SC_DEC))


target = dt.sc_unsigned(137)
target.__assign__(INITIAL)
target_before = decimal(target)
input_stream = STD.istringstream("")
input_stream.exceptions(STD.ios.failbit)
try:
    input_stream >> target
except STD.exception as error:
    # cppyy translates this standard-library exception generically; the
    # fail/eof state and installed failbit mask establish its native source.
    assert str(error.what()) == "std::exception"
else:
    raise AssertionError("exhausted input did not throw with failbit masked")

input_failure_state = int(input_stream.rdstate())
assert input_failure_state == int(STD.ios.eofbit | STD.ios.failbit)
assert int(input_stream.exceptions()) == int(STD.ios.failbit)
assert decimal(target) == target_before, "failed extraction changed the destination"

# Clear and reload the same native stream while keeping its failbit mask. Valid
# chained extractions update two live sc_dt targets and return the same stream.
input_stream.clear()
input_stream.str(f"{RECOVERED} 42")
tail = dt.sc_unsigned(32)
tail.__assign__(0)
first_extract = input_stream >> target
second_extract = first_extract >> tail
assert first_extract is input_stream
assert second_extract is input_stream
assert decimal(target) == str(RECOVERED)
assert decimal(tail) == "42"
assert bool(input_stream)
assert int(input_stream.rdstate()) == int(STD.ios.eofbit)

# A real badbit exception mask rejects insertion into the failed output stream.
# Clearing that state recovers the same stream, and later insertion chains.
output_stream = STD.ostringstream()
output_stream.exceptions(STD.ios.badbit)
try:
    output_stream.setstate(STD.ios.badbit)
except STD.exception as error:
    assert str(error.what()) == "std::exception"
else:
    raise AssertionError("setting badbit did not throw with badbit masked")
assert int(output_stream.rdstate()) == int(STD.ios.badbit)
assert int(output_stream.exceptions()) == int(STD.ios.badbit)

try:
    output_stream << target
except STD.exception as error:
    assert str(error.what()) == "std::exception"
else:
    raise AssertionError("insertion into bad output stream did not throw")
output_failure_state = int(output_stream.rdstate())
assert output_failure_state == int(STD.ios.badbit | STD.ios.failbit)

output_stream.clear()
first_insert = output_stream << target
second_insert = first_insert << tail
assert first_insert is output_stream
assert second_insert is output_stream
assert str(output_stream.str()) == f"{RECOVERED}42"
assert int(output_stream.rdstate()) == 0

print("STREAM_EXCEPTION_INPUT_RECOVERED fail|eof eof width=137")
print("STREAM_EXCEPTION_OUTPUT_RECOVERED bad|fail good")
print("STREAM_EXCEPTION_CHAINED_SAME_OWNER")
