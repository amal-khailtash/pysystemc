"""Preserve exact Python-integer and native-source integer construction."""

from pathlib import Path
import cppyy

cppyy.include(str(Path(__file__).with_name("integers.h")))


def _integer_text(value):
    """Render an integer exactly without changing Python's digit-limit setting."""
    negative = value < 0
    remaining = abs(value)
    parts = []
    while remaining:
        remaining, part = divmod(remaining, 1_000_000_000)
        parts.append(part)
    if not parts:
        return "0"
    return ("-" if negative else "") + str(parts[-1]) + "".join(f"{part:09d}" for part in reversed(parts[:-1]))


def _constructor(native_type):
    """Select exact constructors without changing the native integer class."""
    native = native_type.__init__
    string_constructor = native.__overload__("const char*")
    copy_constructor = native.__overload__(f"const {native_type.__cpp_name__}&")
    native_assignment = native_type.__assign__
    string_assignment = native_assignment.__overload__("const char*")

    def construct(owner, *args, **kwargs):
        """Initialize owned native integer storage from an exact value.

        Args:
            args: Native constructor arguments. Python integers use exact
                decimal text, preserving all bits before native width wrapping.
                Native value/generic sources use exact C++ overload resolution.
            kwargs: Native single-source keyword arguments (a or v) use the
                same exact conversion; other native arguments pass through.
        Returns:
            None, matching native construction.
        Raises:
            TypeError: The native constructor or source conversion is unavailable.
            Native exception: Allocation or a configured native report throws.
        Arbitrary template widths remain native. Calls hold the GIL and do not
        suspend. The number owns copied storage; no source references are retained.
        """
        if not args and len(kwargs) == 1 and next(iter(kwargs)) in ("a", "v"):
            args = (next(iter(kwargs.values())),)
            kwargs = {}
        if len(args) == 1 and not kwargs:
            value = args[0]
            if type(value) is int:
                return string_constructor(owner, _integer_text(value))
            if hasattr(type(value), "__cpp_name__"):
                converted = cppyy.gbl.pysystemc_datatypes.IntegerConversion[native_type, type(value)].construct(value)
                return copy_constructor(owner, converted)
        return native(owner, *args, **kwargs)

    construct.__overload__ = native.__overload__
    native_type.__init__ = construct

    def assign(owner, source):
        """Assign exact native integer storage and return the live target.

        Args:
            source: Python integer or native assignment source.
        Returns:
            The target integer, borrowing its existing storage.
        Raises:
            TypeError: Native conversion or assignment is unavailable.
            Native exception: Native conversion/allocation/reporting throws.
        Python integers retain every bit before native width wrapping. Calls
        hold the GIL and retain no source references or callbacks.
        """
        if type(source) is int:
            return string_assignment(owner, _integer_text(source))
        if hasattr(type(source), "__cpp_name__"):
            return cppyy.gbl.pysystemc_datatypes.IntegerConversion[native_type, type(source)].assign(owner, source)
        return native_assignment(owner, source)

    assign.__overload__ = native_assignment.__overload__
    native_type.__assign__ = assign


def _integer_pythonization(native_type, name):
    """Adapt integer template instances on demand for every native width."""
    if name.startswith(("sc_int<", "sc_uint<", "sc_bigint<", "sc_biguint<")):
        _constructor(native_type)


cppyy.py.add_pythonization(_integer_pythonization, "sc_dt")
