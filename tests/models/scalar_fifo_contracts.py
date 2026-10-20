"""Check native-width FIFO callback errors, output preservation and recovery."""

from amal.eda.systemc import sc_core as sc
import cppyy
import math


class Owner:
    """Provide configurable scalar FIFO values without blocking."""

    def __init__(self):
        """Allocate stable reader event and initial sample."""
        self.value = 0
        self.success = True
        self.changed = sc.sc_event()

    def read(self):
        """Return the selected scalar sample."""
        return self.value

    def nb_read(self):
        """Return the selected success flag and sample."""
        return self.success, self.value

    def num_available(self):
        """Report one available sample."""
        return 1

    def data_written_event(self):
        """Borrow the reader event."""
        return self.changed


name = sc.sc_module_name("top")
top = sc.sc_module(name)
cases = []
for scalar in (
    "signed char",
    "unsigned char",
    "short",
    "unsigned short",
    "int",
    "unsigned int",
    "long",
    "unsigned long",
    "long long",
    "unsigned long long",
):
    owner = Owner()
    port = sc.sc_port[sc.sc_fifo_in_if[scalar]]()
    port(owner)
    cases.append((scalar, owner, port))
additional = []
for scalar in ("bool", "float", "double", "long double"):
    owner = Owner()
    port = sc.sc_port[sc.sc_fifo_in_if[scalar]]()
    port(owner)
    additional.append((scalar, owner, port))
del name
cppyy.cppdef("""
/** Check rejected callbacks leave a borrowed native output unchanged.
 * Python acquires the GIL for callbacks; no waits occur. Owners remain retained.
 * Exceptions are caught only to inspect output preservation; acquired GIL state
 * is released after clearing the Python callback error.
 */
namespace scalar_fifo_check {
 /** Compare successful floating outputs, including NaN and signed zero.
  * The interface and output are borrowed for this call. Python callbacks hold
  * the GIL through the adapter; this helper neither retains storage nor waits.
  */
 template<class T> bool floating_output(sc_core::sc_fifo_in_if<T>& channel, double expected) {
   T output = 7;
   if (!channel.nb_read(output)) return false;
   if (std::isnan(expected)) return std::isnan(output);
   return output == T(expected) && std::signbit(output) == std::signbit(expected);
 }
 template<class T> bool rejected_output(sc_core::sc_fifo_in_if<T>& channel) {
   T output = 7;
   try { channel.nb_read(output); } catch (...) { auto gil = PyGILState_Ensure(); PyErr_Clear();
     PyGILState_Release(gil); return output == T(7); }
   return false;
 }
 template<class T> bool failed_output(sc_core::sc_fifo_in_if<T>& channel) {
   T output = 7;
   return !channel.nb_read(output) && output == T(7);
 }
}
""")
for scalar, owner, port in cases:
    native = port.get_interface()
    bits = cppyy.sizeof(scalar) * 8
    low, high = (0, 2**bits - 1) if scalar.startswith("unsigned") else (-(2 ** (bits - 1)), 2 ** (bits - 1) - 1)
    # char proxies can expose a one-character Python string; native checks below
    # establish numeric output behavior for those specializations.
    for invalid in (True, 1.5, object(), low - 1, high + 1):
        owner.value = invalid
        try:
            native.read()
        except TypeError:
            pass
        else:
            raise AssertionError((scalar, invalid))
        assert cppyy.gbl.scalar_fifo_check.rejected_output[scalar](native), scalar
    owner.value = 0
    owner.success = False
    assert cppyy.gbl.scalar_fifo_check.failed_output[scalar](native), scalar
    owner.success = True
    for value in (low, high):
        owner.value = value
        result = native.read()
        numeric = ord(result) if isinstance(result, str) else result
        if scalar == "signed char" and numeric >= 128:
            numeric -= 256
        assert numeric == value, (scalar, result, value)
for scalar, owner, port in additional:
    native = port.get_interface()
    invalid_values = (0, 1, object()) if scalar == "bool" else (True, "1.25", object())
    for invalid in invalid_values:
        owner.value = invalid
        try:
            native.read()
        except TypeError:
            pass
        else:
            raise AssertionError((scalar, invalid))
        assert cppyy.gbl.scalar_fifo_check.rejected_output[scalar](native), scalar
    owner.value = False if scalar == "bool" else 1.25
    assert native.read() == owner.value
    if scalar != "bool":
        # IEEE non-finite samples must survive the native callback conversion.
        # Check blocking and successful nonblocking reads, then finite recovery.
        for value in (float("inf"), float("-inf"), float("nan"), -0.0):
            owner.value = value
            result = native.read()
            if math.isnan(value):
                assert math.isnan(result), (scalar, result)
            else:
                assert result == value, (scalar, result, value)
                assert math.copysign(1.0, result) == math.copysign(1.0, value)
            assert cppyy.gbl.scalar_fifo_check.floating_output[scalar](native, value), scalar
        owner.value = 1.25
        assert native.read() == 1.25, scalar
    owner.success = False
    assert cppyy.gbl.scalar_fifo_check.failed_output[scalar](native), scalar
sc.sc_start(1, sc.SC_NS)
sc.sc_stop()
print("SCALAR_FIFO_CONTRACTS_OK")
