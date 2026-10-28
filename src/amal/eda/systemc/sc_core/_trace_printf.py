"""Select native printf argument types for formatted trace comments."""

import ctypes
import re
import sys

import cppyy

_CONVERSION = re.compile(
    r"%(?:(\d+)\$)?([-+ #0']*)(\d+|\*(?:\d+\$)?)?(?:\.(\d*|\*(?:\d+\$)?))?(hh|ll|[hlLjzt])?([diouxXfFeEgGaAcspn%])"
)
_SIGNED = {
    "": "int",
    "hh": "int",
    "h": "int",
    "l": "long",
    "ll": "long long",
    "j": "std::intmax_t",
    "z": "std::ptrdiff_t",
    "t": "std::ptrdiff_t",
}
_UNSIGNED = {
    "": "unsigned int",
    "hh": "unsigned int",
    "h": "unsigned int",
    "l": "unsigned long",
    "ll": "unsigned long long",
    "j": "std::uintmax_t",
    "z": "std::size_t",
    "t": "std::size_t",
}


def _count_argument(value, datatype):
    """Borrow aligned count storage, validating writable Python buffers before native access."""
    if value is None or value is cppyy.nullptr or isinstance(value, (int, float, str)):
        raise TypeError("%n requires a writable native pointer or buffer")
    counter = cppyy.gbl.pysystemc_public.trace_count_argument[datatype]
    expected_format = counter.buffer_format()
    expected_ctype = {
        "b": ctypes.c_byte,
        "h": ctypes.c_short,
        "i": ctypes.c_int,
        "l": ctypes.c_long,
        "q": ctypes.c_longlong,
    }[expected_format]
    if isinstance(value, ctypes._Pointer):
        pointee = value._type_
        if not value or ctypes.sizeof(pointee) != cppyy.sizeof(datatype) or pointee is not expected_ctype:
            raise TypeError("%n requires a compatible nonnull count pointer")
        address = ctypes.cast(value, ctypes.c_void_p).value
        if address % counter.alignment():
            raise TypeError("%n requires aligned count storage")
        wrapper = counter.from_address(address)
        wrapper._buffer_owner = value
        return wrapper
    try:
        view = memoryview(value)
    except TypeError:
        referent = getattr(value, "_obj", None)
        if type(referent) is not expected_ctype and not (
            isinstance(referent, ctypes.Array) and referent._type_ is expected_ctype
        ):
            raise TypeError("%n requires a writable native count pointer or compatible buffer")
        address = ctypes.cast(value, ctypes.c_void_p).value
        start = ctypes.addressof(referent)
        if not start <= address <= start + ctypes.sizeof(referent) - cppyy.sizeof(datatype):
            raise TypeError("%n count pointer exceeds its referenced storage")
        if address % counter.alignment():
            raise TypeError("%n requires aligned count storage")
        wrapper = counter.from_address(address)
    else:
        size = cppyy.sizeof(datatype)
        native_prefix = "<" if sys.byteorder == "little" else ">"
        buffer_format = view.format.removeprefix(native_prefix).lstrip("@=")
        ctypes_alias = isinstance(value, ctypes.Array) and value._type_ is expected_ctype
        if (
            view.readonly
            or not view.c_contiguous
            or view.itemsize != size
            or view.nbytes < size
            or (buffer_format != expected_format and not ctypes_alias)
        ):
            raise TypeError("%n requires compatible writable contiguous count storage")
        address = ctypes.addressof(ctypes.c_char.from_buffer(view))
        if address % counter.alignment():
            raise TypeError("%n requires aligned count storage")
        wrapper = counter.from_address(address)
    wrapper._buffer_owner = value
    return wrapper


def tprintf(file, format, *args):
    """Write a native C printf formatted trace comment.

    Args:
        file: Borrowed open trace file or a native null pointer for a no-op.
        format: C printf format string, including width, precision and lengths.
        *args: Numeric, string or native pointer arguments matching conversions.

    Returns:
        None after writing the comment without advancing simulation.

    Raises:
        TypeError: Argument count or native argument conversion is invalid.
        ValueError: Invalid format, length or mixed positional argument forms.
        RuntimeError: Native formatting fails.

    The GIL remains held and arguments live through formatting. Native integer
    widths follow C length modifiers; strings use retained native storage. The
    adapter allocates sufficient buffer space for comments beyond 4096 bytes.
    Writable %n arguments must be native pointers or compatible writable buffers.
    Close the file with its matching format-specific close helper exactly once.
    """
    if not isinstance(format, str) or "\0" in format:
        raise ValueError("format must be a string without NUL characters")
    types = []
    values = []
    pieces = []
    used = set()
    index = 0
    cursor = 0
    positional = None

    def take(position=None):
        """Retain an argument while enforcing native positional format rules."""
        nonlocal index, positional
        numbered = position is not None
        if positional is not None and positional != numbered:
            raise ValueError("cannot mix positional and sequential printf arguments")
        positional = numbered
        selected = int(position) - 1 if numbered else index
        if not numbered:
            index += 1
        if selected < 0 or selected >= len(args):
            raise TypeError("not enough printf arguments")
        used.add(selected)
        return args[selected]

    while (offset := format.find("%", cursor)) != -1:
        pieces.append(format[cursor:offset])
        match = _CONVERSION.match(format, offset)
        if match is None:
            raise ValueError("unsupported native printf conversion")
        position, flags, width, precision, length, conversion = match.groups()
        length = length or ""
        cursor = match.end()
        if conversion == "%":
            if match.group() != "%%":
                raise ValueError("literal percent must use %%")
            pieces.append("%%")
            continue
        if conversion == "n" and (flags or width is not None or precision is not None):
            raise ValueError("%n does not accept flags, width or precision")
        normalized_fields = []
        for field in (width, precision):
            if field is not None and field.startswith("*"):
                types.append("int")
                values.append(take(field[1:-1] if field.endswith("$") else None))
                normalized_fields.append("*")
            else:
                normalized_fields.append(field)
        value = take(position)
        if conversion in "di":
            datatype = _SIGNED.get(length)
        elif conversion in "ouxX":
            datatype = _UNSIGNED.get(length)
        elif conversion in "fFeEgGaA":
            datatype = {"": "double", "l": "double", "L": "long double"}.get(length)
        elif conversion == "s":
            datatype = {"": "std::string", "l": "std::wstring"}.get(length)
            if datatype:
                value = getattr(cppyy.gbl.std, datatype.split("::")[1])(value)
        elif conversion == "c":
            datatype = {"": "int", "l": "unsigned int"}.get(length)
            if isinstance(value, str):
                if len(value) != 1:
                    raise TypeError("%c requires one character")
                value = ord(value)
        elif conversion == "n":
            datatype = {
                "": "int",
                "hh": "signed char",
                "h": "short",
                "l": "long",
                "ll": "long long",
                "j": "std::intmax_t",
                "z": "std::ptrdiff_t",
                "t": "std::ptrdiff_t",
            }.get(length)
            if datatype:
                value = _count_argument(value, datatype)
                datatype = f"pysystemc_public::trace_count_argument<{datatype}>"
        else:
            datatype = "const void*" if not length else None
            if value is None:
                value = cppyy.nullptr
        if datatype is None:
            raise ValueError("invalid native printf length modifier")
        types.append(datatype)
        values.append(value)
        width, precision = normalized_fields
        pieces.append(
            "%" + flags + (width or "") + ("." + precision if precision is not None else "") + length + conversion
        )
    pieces.append(format[cursor:])
    if len(used) != len(args):
        raise TypeError("unused printf arguments")
    format = "".join(pieces)
    operation = cppyy.gbl.pysystemc_public.trace_printf
    if types:
        operation = operation[tuple(types)]
    operation(cppyy.nullptr if file is None else file, format, *values)
