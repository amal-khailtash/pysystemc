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
from . import _bit  # noqa: E402, F401

_native_remainder = getattr(_namespace.sc_signed, "__mod__", lambda self, other: NotImplemented)
_native_left_shift = getattr(_namespace.sc_unsigned, "__lshift__", lambda self, other: NotImplemented)
_native_right_shift = getattr(_namespace.sc_unsigned, "__rshift__", lambda self, other: NotImplemented)
sc_context = cppyy.gbl.pysystemc_datatypes.context_owner
sc_fxtype_context = sc_context[_namespace.sc_fxtype_params]
sc_fxcast_context = sc_context[_namespace.sc_fxcast_switch]
sc_length_context = sc_context[_namespace.sc_length_param]
_native_to_string = _namespace.to_string


def to_string(value):
    """Return the native spelling for a datatype policy or format enum.

    Args:
        value: A native ``sc_numrep``, ``sc_fmt``, ``sc_enc``, ``sc_q_mode``,
            ``sc_o_mode`` or ``sc_switch`` enum value.
    Returns:
        A Python string copied from the exact native enum overload.
    Raises:
        TypeError: Native conversion or overload selection fails.
        Native exception: Native string allocation fails.
    Calls hold the GIL and do not enter simulation. Unknown numeric enum values
    retain the native spelling rather than adding Python validation.
    """
    enum_type = type(value)
    if getattr(enum_type, "__cpp_name__", "") in {
        "sc_dt::sc_numrep",
        "sc_dt::sc_fmt",
        "sc_dt::sc_enc",
        "sc_dt::sc_q_mode",
        "sc_dt::sc_o_mode",
        "sc_dt::sc_switch",
    }:
        return str(cppyy.gbl.pysystemc_datatypes.enum_string[enum_type](value))
    return _native_to_string(value)


to_string.__overload__ = _native_to_string.__overload__


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
