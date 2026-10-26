"""Preserve exact native complements and four-state vector operator dispatch."""

from pathlib import Path

import cppyy

cppyy.include(str(Path(__file__).with_name("operators.h")))


def _native_argument_type(value):
    """Resolve a native argument type without restricting datatype widths."""
    if hasattr(type(value), "__cpp_name__"):
        return type(value)
    if type(value) is bool:
        return "bool"
    if type(value) is int:
        return "int"
    if type(value) is str:
        return "std::string"
    raise TypeError("Expected a native datatype or a bool, int or bit string")


def _complement(native_type):
    """Install owned native complement results on a concrete scalar type."""
    operation = cppyy.gbl.pysystemc_datatypes.unary_operator[native_type].invert

    def invert(value):
        """Return an owned native complement without changing the input.

        Returns:
            An independent native value with native width and sign semantics.
        Raises:
            Native exception: Allocation or a configured native report fails.
        Calls hold the GIL and do not suspend or retain input storage.
        """
        return operation(value)

    native_type.__invert__ = invert


for _type in (cppyy.gbl.sc_dt.sc_bit, cppyy.gbl.sc_dt.sc_signed, cppyy.gbl.sc_dt.sc_unsigned):
    _complement(_type)


def _vector_operator(native_method, reverse=False, inplace=False):
    """Build a native expression adapter without choosing a scalar overload."""

    def apply(owner, other):
        """Return the exact native vector bitwise expression result.

        Args:
            other: Native datatype, bool/int or native-compatible bit string.
        Returns:
            An owned native result retaining native width and four-state rules,
            or the same mutated owner for a compound operation.
        Raises:
            TypeError: Argument conversion or native expression selection fails.
            Native exception: Allocation or a configured report fails.
        Inputs are borrowed for the call. Calls hold the GIL and do not suspend.
        """
        left, right = (other, owner) if reverse else (owner, other)
        operation = cppyy.gbl.pysystemc_datatypes.binary_operator[
            _native_argument_type(left), _native_argument_type(right)
        ]
        result = getattr(operation, native_method)(left, right)
        return owner if inplace else result

    return apply


for _type in (cppyy.gbl.sc_dt.sc_bv_base, cppyy.gbl.sc_dt.sc_lv_base):
    for _method, _native in (("and", "bit_and"), ("or", "bit_or"), ("xor", "bit_xor")):
        setattr(_type, f"__{_method}__", _vector_operator(_native))
        setattr(_type, f"__r{_method}__", _vector_operator(_native, reverse=True))

        setattr(_type, f"__i{_method}__", _vector_operator(f"compound_{_method}", inplace=True))


def _bitref_bool(value):
    """Read a live native integer bit reference as a Python boolean.

    Returns:
        bool: The current bit value, preserving native bool and ! semantics.
    Raises:
        Native exception: A configured report fails.
    The proxy and its parent must remain valid. Calls hold the GIL and do not
    suspend, copy the borrowed view or acquire ownership of native pool storage.
    """
    return bool(value.to_bool())


for _name in ("sc_int", "sc_uint", "sc_signed", "sc_unsigned"):
    for _suffix in ("_bitref", "_bitref_r"):
        getattr(cppyy.gbl.sc_dt, _name + _suffix).__bool__ = _bitref_bool
