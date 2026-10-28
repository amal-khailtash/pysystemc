"""Select defined native concatenation overloads for boolean bit operands."""

from pathlib import Path

import cppyy

cppyy.include(str(Path(__file__).with_name("concat_adapters.h")))
_native_concat = cppyy.gbl.sc_dt.concat


def _is_vector_bit(value):
    """Recognize arbitrary native vector bit proxy instantiations."""
    return getattr(type(value), "__cpp_name__", "").startswith(("sc_dt::sc_bitref<", "sc_dt::sc_bitref_r<"))


def concat(left, right):
    """Return a native concatenation view over live operands.

    Args:
        left: Native integer/vector/proxy or a native-compatible scalar.
        right: Native integer/vector/proxy or a native-compatible scalar.
    Returns:
        A native concatenation preserving native length and mutability. Boolean
        vector-bit forms use the defined binary-vector result overload, selected
        explicitly around incompatible upstream forward declarations.
    Raises:
        TypeError: No native overload accepts the operands.
        Native exception: Allocation or a configured native report throws.
    Keep all borrowed operand parents live while using the result. Native shell
    and temporary-pool lifetime rules apply. Calls hold the GIL and do not
    suspend or retain Python callbacks. Exact native overload selection remains
    available through ``concat.__overload__``.
    """
    if type(right) is bool and _is_vector_bit(left):
        return cppyy.gbl.pysystemc_datatypes.BoolBitConcat[type(left)].tail(left, right)
    if type(left) is bool and _is_vector_bit(right):
        return cppyy.gbl.pysystemc_datatypes.BoolBitConcat[type(right)].head(left, right)
    if type(right) is str:
        return cppyy.gbl.pysystemc_datatypes.ConcatText[type(left)].tail(left, right)
    if type(left) is str:
        return cppyy.gbl.pysystemc_datatypes.ConcatText[type(right)].head(left, right)
    return _native_concat(left, right)


class _ConcatCallable:
    """Retain native template and overload selectors with adapted calls."""

    __doc__ = concat.__doc__
    __name__ = "concat"
    __qualname__ = "concat"
    __call__ = staticmethod(concat)

    def __getitem__(self, parameters):
        """Select native template arguments without restricting owner types."""
        return _native_concat[parameters]

    def __overload__(self, signature):
        """Select an exact native signature with its native preconditions."""
        return _native_concat.__overload__(signature)


concat = _ConcatCallable()
