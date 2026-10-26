"""Select exact native shift expressions for numeric values and count types."""

from pathlib import Path
import cppyy
from ._operators import _native_argument_type

cppyy.include(str(Path(__file__).with_name("shift_adapters.h")))


def _shift(direction):
    """Build a native shift wrapper preserving value and count types."""

    def apply(value, count):
        """Return an independently owned native shifted value.

        Args:
            count: Native integral/count datatype, or Python int (native int).
        Returns:
            The exact native shift result, with native width and sign rules.
        Raises:
            TypeError: Count conversion or the native expression is unavailable.
            Native exception: Allocation or configured reporting throws.
        Value and count are borrowed for a GIL-held call. Native shift-count
        preconditions apply. No waits, callbacks or source references persist.
        """
        operation = cppyy.gbl.pysystemc_datatypes.ShiftOperators[type(value), _native_argument_type(count)]
        return getattr(operation, direction)(value, count)

    return apply


def _adapt(native_type):
    """Install both shift expressions on a native numeric class."""
    native_type.__lshift__ = _shift("left")
    native_type.__rshift__ = _shift("right")


def _pythonize(native_type, name):
    """Preserve native shifts for arbitrary derived numeric template widths."""
    if name.startswith(("sc_bigint<", "sc_biguint<", "sc_fixed<", "sc_ufixed<", "sc_fixed_fast<", "sc_ufixed_fast<")):
        _adapt(native_type)


for _name in (
    "sc_signed",
    "sc_unsigned",
    "sc_concatref",
    "sc_fxval",
    "sc_fxval_fast",
    "sc_fxnum",
    "sc_fxnum_fast",
    "sc_fix",
    "sc_ufix",
    "sc_fix_fast",
    "sc_ufix_fast",
):
    _adapt(getattr(cppyy.gbl.sc_dt, _name))
cppyy.py.add_pythonization(_pythonize, "sc_dt")
