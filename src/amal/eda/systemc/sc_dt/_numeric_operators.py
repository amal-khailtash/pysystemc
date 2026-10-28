"""Preserve exact native arithmetic, comparison and compound operand types."""

from pathlib import Path

import cppyy

from ._operators import _native_argument_type
from ._integers import _integer_text
from ._native_methods import NativeMethod

cppyy.include(str(Path(__file__).with_name("numeric_operators.h")))


def _argument_type(value):
    """Select a native scalar or datatype without imposing a width registry."""
    if type(value) is int and not -(1 << 31) <= value < (1 << 31):
        return "sc_dt::int64" if value < (1 << 63) else "sc_dt::uint64"
    return "double" if type(value) is float else _native_argument_type(value)


def named_function(name):
    """Build an exact native output-parameter arithmetic callable."""
    native = getattr(cppyy.gbl.sc_dt, name)

    def apply(target, left, right):
        """Write native arithmetic into existing output storage.

        Args:
            target: Mutable native fixed-point output owner.
            left: Native numeric operand or Python scalar.
            right: Native numeric operand or Python scalar.
        Returns:
            None, matching the native output-parameter function.
        Raises:
            TypeError: The native overload or operand conversion is unavailable.
            Native exception: Allocation or configured arithmetic reports throw.
        Calls hold the GIL. All operands are borrowed for the call; native
        aliasing, precision, observer and arithmetic preconditions apply.
        Arbitrary-precision sc_fxnum and sc_fxval outputs must not alias
        either input owner. Their native named overloads can delete or replace
        output representation before completing a calculation, invalidating
        an aliased input read. Fast fixed-point outputs retain their
        overload-specific native alias behavior; no blanket alias guarantee is
        made.
        """
        adapter = cppyy.gbl.pysystemc_datatypes.NumericFunctions[
            type(target), _argument_type(left), _argument_type(right)
        ]
        getattr(adapter, "perform_" + name)(target, left, right)

    apply.__overload__ = native.__overload__
    apply.__name__ = name
    apply.__qualname__ = name
    return apply


def _binary(operation, reverse=False):
    """Build a native expression wrapper with exact argument dispatch."""

    def apply(owner, other):
        """Return an independent native result or a comparison boolean.

        Args:
            other: Native numeric operand, Python int, float or native bit string.
        Returns:
            The native expression result with native widths and precision.
        Raises:
            TypeError: Native argument conversion or the expression is unavailable.
            Native exception: Allocation or configured arithmetic reports throw.
        Both operands are borrowed for the GIL-held call. No source references,
        waits or callbacks persist. Python ints use native C++ int when they
        fit, otherwise a signed or unsigned native 64-bit scalar;
        Python floats use double. Native arithmetic preconditions apply.
        """
        left, right = (other, owner) if reverse else (owner, other)
        adapter = cppyy.gbl.pysystemc_datatypes.NumericOperators[_argument_type(left), _argument_type(right)]
        return getattr(adapter, operation)(left, right)

    return apply


def _compound(operation):
    """Build a native compound expression on the existing numeric owner."""

    def apply(owner, source):
        """Mutate numeric storage and return the original Python owner.

        Args:
            source: Native numeric operand or an exact Python scalar type.
        Returns:
            The existing owner, with native wrapping and precision rules.
        Raises:
            TypeError: Native conversion or the compound expression is unavailable.
            Native exception: Configured reports or allocation throw.
        Owners remain live for a GIL-held call. No callbacks or references persist.
        Native arithmetic and count preconditions apply.
        """
        if operation == "assign" and type(source) is int and not -(1 << 31) <= source < (1 << 31):
            source = _integer_text(source)
        adapter = cppyy.gbl.pysystemc_datatypes.NumericMutation[type(owner), _argument_type(source)]
        getattr(adapter, operation)(owner, source)
        return owner

    return apply


_ARITHMETIC = {"add": "add", "sub": "subtract", "mul": "multiply", "truediv": "divide"}
_BITS = {"mod": "remainder", "and": "bit_and", "or": "bit_or", "xor": "bit_xor"}
_COMPARISONS = {
    "eq": "equal",
    "ne": "not_equal",
    "lt": "less",
    "le": "less_equal",
    "gt": "greater",
    "ge": "greater_equal",
}


def _install(native_type, name, wrapper):
    """Keep exact native overload selectors available alongside public dispatch."""
    original = getattr(native_type, name, None)
    setattr(native_type, name, NativeMethod(wrapper, original))


def _adapt(native_type):
    """Adapt native expressions while retaining inherited template behavior."""
    integer = any(
        issubclass(native_type, getattr(cppyy.gbl.sc_dt, name))
        for name in ("sc_signed", "sc_unsigned", "sc_int_base", "sc_uint_base", "sc_concatref")
    )
    fixed = any(
        issubclass(native_type, getattr(cppyy.gbl.sc_dt, name))
        for name in ("sc_fix", "sc_ufix", "sc_fix_fast", "sc_ufix_fast")
    )
    if any(
        issubclass(native_type, getattr(cppyy.gbl.sc_dt, name))
        for name in ("sc_fxval", "sc_fxval_fast", "sc_fxnum", "sc_fxnum_fast")
    ):
        _install(native_type, "__assign__", _compound("assign"))
    for name, operation in _ARITHMETIC.items():
        _install(native_type, f"__{name}__", _binary(operation))
        _install(native_type, f"__r{name}__", _binary(operation, reverse=True))
        if hasattr(native_type, f"__i{name}__"):
            _install(native_type, f"__i{name}__", _compound(operation))
    for name, operation in _BITS.items():
        if integer or (fixed and name != "mod") or hasattr(native_type, f"__{name}__"):
            _install(native_type, f"__{name}__", _binary(operation))
            _install(native_type, f"__r{name}__", _binary(operation, reverse=True))
        if hasattr(native_type, f"__i{name}__"):
            _install(native_type, f"__i{name}__", _compound(operation))
    for name, operation in _COMPARISONS.items():
        _install(native_type, f"__{name}__", _binary(operation))
    for name, operation in (("lshift", "left_shift"), ("rshift", "right_shift")):
        if hasattr(native_type, f"__i{name}__"):
            _install(native_type, f"__i{name}__", _compound(operation))


def _pythonize(native_type, name):
    """Preserve expression types on arbitrary native numeric templates."""
    if name.startswith(
        (
            "sc_bigint<",
            "sc_biguint<",
            "sc_int<",
            "sc_uint<",
            "sc_fixed<",
            "sc_ufixed<",
            "sc_fixed_fast<",
            "sc_ufixed_fast<",
        )
    ):
        _adapt(native_type)


for _name in (
    "sc_signed",
    "sc_unsigned",
    "sc_int_base",
    "sc_uint_base",
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
