"""Exercise guarded FIFO value and output reads for a copy-only native value."""

from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy


cppyy.include(str(Path(__file__).resolve().parents[1] / "native" / "sc_core_fifo_nondefault.h"))
fixture = cppyy.gbl.sc_core_fifo_nondefault
value_type = fixture.Value
reader = fixture.Reader()
interface_type = sc.sc_fifo_blocking_in_if[value_type]
interface = cppyy.bind_object(fixture.blocking_input_address(reader), interface_type)


class InvalidOutput:
    """Provide an incompatible initial value to prove conversion precedes read."""

    value = object()


value = interface.read()
assert isinstance(value, value_type)
assert value.value == 17
assert bool(value.__python_owns__), "by-value read must return independent native storage"

try:
    interface.read(InvalidOutput())
except TypeError:
    pass
else:
    raise AssertionError("incompatible output initial value was accepted")
assert reader.value_read_calls() == 1
assert reader.output_read_calls() == 0

output = value_type(-1)
result = interface.read(output)
assert result is None
assert output.value == 23
assert reader.value_read_calls() == 1
assert reader.output_read_calls() == 1
print(
    "FIFO_NONDEFAULT_REFERENCE",
    value.value,
    output.value,
    reader.value_read_calls(),
    reader.output_read_calls(),
)
