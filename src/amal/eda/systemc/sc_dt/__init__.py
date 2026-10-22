"""Native SystemC integers, logic vectors and fixed-point datatypes.

Templates are instantiated by cppyy on demand. Fixed-point quantization and
overflow policies are the native sc_dt enums, not Python substitutes.
"""

from pathlib import Path

import cppyy
from .._namespace import (
    public_names as _public_names,
    document_member as _document_member,
    namespace_help as _namespace_help,
)

_namespace = cppyy.gbl.sc_dt
cppyy.include(str(Path(__file__).with_name("adapters.h")))
_native_remainder = getattr(_namespace.sc_signed, "__mod__", lambda self, other: NotImplemented)
_native_left_shift = getattr(_namespace.sc_unsigned, "__lshift__", lambda self, other: NotImplemented)
_native_right_shift = getattr(_namespace.sc_unsigned, "__rshift__", lambda self, other: NotImplemented)
sc_context = cppyy.gbl.pysystemc_datatypes.context_owner
sc_fxtype_context = sc_context[_namespace.sc_fxtype_params]
sc_fxcast_context = sc_context[_namespace.sc_fxcast_switch]


def _signed_remainder(self, other):
    """Return native signed remainder, preserving sign, widths and report errors."""
    if not isinstance(other, _namespace.sc_signed):
        return _native_remainder(self, other)
    return cppyy.gbl.pysystemc_datatypes.remainder(self, other)


def _unsigned_left_shift(self, count):
    """Return independent native unsigned storage shifted by an int count."""
    if not isinstance(count, int):
        return _native_left_shift(self, count)
    return cppyy.gbl.pysystemc_datatypes.left_shift(self, count)


def _unsigned_right_shift(self, count):
    """Return independent native unsigned storage using native right-shift widths."""
    if not isinstance(count, int):
        return _native_right_shift(self, count)
    return cppyy.gbl.pysystemc_datatypes.right_shift(self, count)


_namespace.sc_signed.__mod__ = _signed_remainder
_namespace.sc_unsigned.__lshift__ = _unsigned_left_shift
_namespace.sc_unsigned.__rshift__ = _unsigned_right_shift
__all__ = _public_names(_namespace)


def __getattr__(name):
    """Resolve and cache a native datatype, template, enum or function.

    Raises AttributeError for names absent from the native sc_dt namespace.
    """
    if name.startswith("_"):
        raise AttributeError(name)
    member = _document_member(getattr(_namespace, name))
    globals()[name] = member
    return member


def __dir__():
    """List Python exports and discoverable native sc_dt members."""
    return sorted(set(globals()) | set(_public_names(_namespace)))


__doc__ = _namespace_help(globals(), _namespace)
