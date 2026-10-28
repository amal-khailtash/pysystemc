"""Preserve native legacy string formatting and character mutation."""

from pathlib import Path
import cppyy

cppyy.include(str(Path(__file__).with_name("string_adapters.h")))
_type = cppyy.gbl.sc_dt.sc_string_old
_native_fmt = _type.fmt


def _argument(value):
    """Select explicit native formatting types without parsing format strings."""
    if isinstance(value, str):
        return cppyy.gbl.std.string, cppyy.gbl.std.string(value)
    if type(value) is int:
        return "int", value
    if type(value) is float:
        return "double", value
    return type(value), value


def _format(format, *args):
    """Return a native formatted string with independent storage.

    Args:
        format: Native printf format string.
        args: Matching native printf arguments. Python int uses C++ int, float
            uses double, and str uses null-terminated bytes. Explicit cppyy
            scalar types may be used for other native specifiers.
    Returns:
        sc_string_old: Independent native string storage.
    Raises:
        TypeError: Argument conversion or template instantiation fails.
        Native exception: Native allocation or a configured report throws.
    Matching formats and argument types are native preconditions. Calls hold
    the GIL, never suspend, and retain no argument references or callbacks.
    """
    converted = [_argument(value) for value in args]
    return cppyy.gbl.pysystemc_datatypes.string_format[tuple(kind for kind, _ in converted)](
        format, *(value for _, value in converted)
    )


def _fmt(owner, value):
    """Apply one native format argument and return the live string.

    Args:
        value: Matching native printf value, with the same type rules as to_string.
    Returns:
        sc_string_old: The mutated owner.
    Raises:
        TypeError: Native argument conversion fails.
        Native exception: Native allocation or configured reporting throws.
    Native format/type preconditions apply. The GIL stays held and no source
    references or callbacks are retained beyond the call.
    """
    kind, converted = _argument(value)
    return cppyy.gbl.pysystemc_datatypes.string_fmt[kind](owner, converted)


def _setitem(owner, index, value):
    """Set one character with native copy-on-write semantics.

    Args:
        index: Valid native character position; mutable indexing is unchecked.
        value: One native char.
    Returns:
        None.
    Raises:
        Native exception: Native allocation or configured reporting throws.
        TypeError: Index or character conversion fails.
    Valid bounds are a native precondition. Calls hold the GIL and retain no
    references or callbacks.
    """
    return cppyy.gbl.pysystemc_datatypes.string_setitem(owner, index, value)


def _radd(owner, source):
    """Return independent native concatenation of source bytes and this string.

    Args:
        source: Native null-terminated character string.
    Returns:
        sc_string_old: Independently owned concatenation.
    Raises:
        TypeError: Source conversion fails.
        Native exception: Native allocation or configured reporting throws.
    Calls hold the GIL and borrow sources only for the call.
    """
    return cppyy.gbl.pysystemc_datatypes.string_radd(source, owner)


_fmt.__overload__ = _native_fmt.__overload__
_type.to_string = staticmethod(_format)
_type.fmt = _fmt
_type.__setitem__ = _setitem
_type.__radd__ = _radd
