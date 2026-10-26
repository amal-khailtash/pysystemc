"""Preserve exact native complements and four-state vector operator dispatch."""

from pathlib import Path

import cppyy

from ._native_methods import NativeMethod

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


def _vector_argument_type(value):
    """Resolve native vector operands, including borrowed pointer arrays.

    cppyy represents a logic pointer as a logic proxy and a bool pointer as a
    LowLevelView. Native vector overloads consume these as arrays; callers must
    retain storage with at least the vector's native length. No bounds or
    lifetime validation is added. Scalar logic expressions use their own path.
    """
    if isinstance(value, cppyy.gbl.sc_dt.sc_logic):
        return "const sc_dt::sc_logic*"
    if type(value).__module__ == "cppyy" and type(value).__name__ == "LowLevelView" and value.typecode == "?":
        return "const bool*"
    return _native_argument_type(value)


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


def _scalar_bit_operator(operation, reverse=False):
    """Select native bit/logic expressions with exact scalar operand types."""

    def apply(owner, other):
        """Return a native bit/logic value or comparison boolean.

        Args:
            other: Native bit/logic value, bool, int or single-character string.
        Returns:
            An independently owned native value or copied comparison boolean.
        Raises:
            TypeError: The native expression or scalar conversion is unavailable.
            Native exception: Allocation or configured value reports throw.
        Calls hold the GIL and borrow inputs only during the expression. Native
        truth tables and invalid-value reports apply; no callbacks are retained.
        """
        if any(getattr(base, "__cpp_name__", "").startswith("sc_dt::sc_proxy<") for base in type(other).__mro__):
            return _vector_operator(operation, reverse=reverse)(owner, other)
        left, right = (other, owner) if reverse else (owner, other)
        left_type = "char" if type(left) is str else _native_argument_type(left)
        right_type = "char" if type(right) is str else _native_argument_type(right)
        adapter = cppyy.gbl.pysystemc_datatypes.binary_operator[left_type, right_type]
        return getattr(adapter, operation)(left, right)

    return apply


for _type in (cppyy.gbl.sc_dt.sc_bit, cppyy.gbl.sc_dt.sc_logic):
    for _method, _operation in (("and", "bit_and"), ("or", "bit_or"), ("xor", "bit_xor")):
        for _name, _reverse in ((f"__{_method}__", False), (f"__r{_method}__", True)):
            setattr(_type, _name, NativeMethod(_scalar_bit_operator(_operation, _reverse), getattr(_type, _name, None)))
    for _name, _operation in (("__eq__", "equal"), ("__ne__", "not_equal")):
        setattr(_type, _name, NativeMethod(_scalar_bit_operator(_operation), getattr(_type, _name, None)))


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
            _vector_argument_type(left), _vector_argument_type(right)
        ]
        result = getattr(operation, native_method)(left, right)
        return owner if inplace else result

    return apply


def _adapt_vector(native_type):
    """Preserve exact vector and arbitrary CRTP proxy expression operands."""
    for _method, _native in (("and", "bit_and"), ("or", "bit_or"), ("xor", "bit_xor")):
        for name, wrapper in (
            (f"__{_method}__", _vector_operator(_native)),
            (f"__r{_method}__", _vector_operator(_native, reverse=True)),
            (f"__i{_method}__", _vector_operator(f"compound_{_method}", inplace=True)),
        ):
            if name.startswith("__i") and native_type.__cpp_name__.startswith(
                ("sc_dt::sc_bitref<", "sc_dt::sc_bitref_r<")
            ):
                continue
            setattr(native_type, name, NativeMethod(wrapper, getattr(native_type, name, None)))
    for name, operation in (("__eq__", "equal"), ("__ne__", "not_equal")):
        setattr(native_type, name, NativeMethod(_vector_operator(operation), getattr(native_type, name, None)))


def _pythonize_vector(native_type, name):
    """Adapt native sc_proxy instantiations for unrestricted CRTP owners."""
    if name.startswith(
        ("sc_proxy<", "sc_bitref<", "sc_bitref_r<", "sc_subref<", "sc_subref_r<", "sc_concref<", "sc_concref_r<")
    ):
        _adapt_vector(native_type)


cppyy.py.add_pythonization(_pythonize_vector, "sc_dt")
for _type in (cppyy.gbl.sc_dt.sc_bv_base, cppyy.gbl.sc_dt.sc_lv_base):
    _adapt_vector(_type)


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


def _subref_assignment(native):
    """Preserve native generic-source assignment on integer range references."""

    def assign(owner, other):
        """Assign a native value source through the selected range.

        Args:
            other: Native datatype/generic source or native-compatible scalar.
        Returns:
            The same mutable range view borrowing its live parent.
        Raises:
            TypeError: The native assignment/conversion is unavailable.
            Native exception: A configured report throws.
        Calls hold the GIL and never suspend; no source storage is retained.
        """
        if hasattr(type(other), "__cpp_name__"):
            operation = cppyy.gbl.pysystemc_datatypes.binary_operator[type(owner), type(other)]
            operation.assign(owner, other)
            return owner
        return native(owner, other)

    assign.__overload__ = native.__overload__
    return assign


for _name in ("sc_int_subref", "sc_uint_subref", "sc_signed_subref", "sc_unsigned_subref"):
    _type = getattr(cppyy.gbl.sc_dt, _name)
    _type.__assign__ = _subref_assignment(_type.__assign__)
