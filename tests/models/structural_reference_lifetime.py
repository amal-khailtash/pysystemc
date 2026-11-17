"""Exercise reference result retention and release on a structural interface."""

import gc
import weakref
from typing import Protocol

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy


Value = sc_dt.sc_biguint[137]
result_references = []
result_metadata = []


@sc.interface(methods={"sample": ("const sc_dt::sc_biguint<137>&", "")})
class SampleInterface(Protocol):
    """Expose a native wide value by const reference."""


class Owner:
    """Create fresh native values without keeping returned values as members."""

    def __init__(self):
        """Initialize the callback sequence."""
        self.next_sample = 0

    def sample(self):
        """Return a fresh high-bit value and record weak lifetime evidence."""
        self.next_sample += 1
        expected = (1 << 136) | (0x10203 * self.next_sample)
        value = Value(expected)
        result_references.append(weakref.ref(value))
        result_metadata.append((cppyy.addressof(value), expected))
        return value


module_name = sc.sc_module_name("top")
top = sc.sc_module(module_name)
owner = Owner()
owner_reference = weakref.ref(owner)
port = sc.sc_port[SampleInterface]("sample_port")
port(owner)
del module_name

native = port.get_interface()
for sample_index in range(3):
    result = native.sample()
    address, expected = result_metadata[sample_index]
    assert cppyy.addressof(result) == address
    assert int(str(result.to_string())) == expected
    assert bool(result[136])
    del result

del owner
gc.collect()
assert owner_reference() is not None, "structural adapter did not retain its implementation"
assert all(reference() is not None for reference in result_references), "reference results were not retained"

sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
assert sc.release_interface_callbacks() == 1
gc.collect()
assert owner_reference() is None, "callback release retained its implementation"
assert all(reference() is None for reference in result_references), "callback release retained reference results"

try:
    native.sample()
except RuntimeError as error:
    assert "released" in str(error)
else:
    raise AssertionError("released structural reference callback accepted a call")

print("STRUCTURAL_REFERENCE_LIFETIME_OK")
