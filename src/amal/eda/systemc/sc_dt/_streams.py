"""Preserve exact native datatype insertion and extraction on live streams."""

from pathlib import Path
import cppyy

cppyy.include(str(Path(__file__).with_name("stream_adapters.h")))
_ostream = cppyy.gbl.std.ostream
_istream = cppyy.gbl.std.istream
_string = cppyy.gbl.std.string


def _datatype(value):
    """Recognize native datatype sources without a specialization registry."""
    return getattr(type(value), "__cpp_name__", "").lstrip(":").startswith("sc_dt::")


def _string_target(value):
    """Recognize the mutable standard string extraction target."""
    return isinstance(value, _string)


def _wrap(native_type, name=""):
    """Adapt current and future native character streams on demand."""
    if issubclass(native_type, _ostream) and not native_type.__dict__.get("_datatype_output_wrapped", False):
        original_insert = native_type.__lshift__

        def insert(stream, value):
            """Insert a native datatype and return the live stream for chaining.

            Args:
                value: Native datatype or an original stream insertion operand.
            Returns:
                The original output stream, retaining its Python owner.
            Raises:
                Native exception: Native reporting, allocation or I/O fails.
                TypeError: Native insertion is unavailable for the operand.
            Both owners are borrowed for the GIL-held call. No waits, callbacks
            or references persist. Native stream state and exception masks apply.
            """
            if _datatype(value):
                cppyy.gbl.pysystemc_datatypes.StreamOperators[type(value)].insert(stream, value)
                return stream
            return original_insert(stream, value)

        native_type.__lshift__ = insert
        native_type._datatype_output_wrapped = True
    if issubclass(native_type, _istream) and not native_type.__dict__.get("_datatype_input_wrapped", False):
        original_extract = native_type.__rshift__

        def extract(stream, value):
            """Extract into native datatype storage and return the live stream.

            Args:
                value: Mutable native target or an original extraction operand.
            Returns:
                The original input stream, retaining its Python owner.
            Raises:
                Native exception: Native reporting, allocation or I/O fails.
                TypeError: The target is immutable or native extraction is unavailable.
            The target and any proxy parent must remain live. Calls hold the GIL
            and never suspend or retain target references. Native stream state,
            exception masks and conversion reports remain in effect.
            """
            if _datatype(value) or _string_target(value):
                cppyy.gbl.pysystemc_datatypes.StreamOperators[type(value)].extract(stream, value)
                return stream
            if isinstance(value, (bool, int, float, str)):
                raise TypeError("stream extraction requires mutable native target")
            return original_extract(stream, value)

        native_type.__rshift__ = extract
        native_type._datatype_input_wrapped = True


for _stream in (
    _ostream,
    _istream,
    cppyy.gbl.std.ostringstream,
    cppyy.gbl.std.istringstream,
    cppyy.gbl.std.stringstream,
    cppyy.gbl.std.ofstream,
    cppyy.gbl.std.ifstream,
):
    _wrap(_stream)
cppyy.py.add_pythonization(_wrap)


def _stringify(value):
    """Return the exact native default stream spelling as independent Python text.

    Args:
        value: Live native numeric datatype or concatenation reference.
    Returns:
        str: Copied representation from a fresh native output stream.
    Raises:
        Native exception: Native stream insertion, reporting or allocation fails.
    Calls hold the GIL and do not suspend or change the value. Borrowed views
    require their parent storage to remain live. No values or callbacks persist.
    """
    return str(cppyy.gbl.pysystemc_datatypes.StreamOperators[type(value)].text(value))


def _wrap_numeric_text(native_type, name):
    """Avoid cppyy's generic numeric string slot invoking adapted shifts."""
    if name.startswith(("sc_int<", "sc_uint<", "sc_bigint<", "sc_biguint<", "sc_fixed<", "sc_ufixed<", "sc_fixed_fast<", "sc_ufixed_fast<")):
        native_type.__str__ = _stringify


for _name in (
    "sc_signed", "sc_unsigned", "sc_int_base", "sc_uint_base", "sc_concatref",
    "sc_fxval", "sc_fxval_fast", "sc_fxnum", "sc_fxnum_fast",
    "sc_fix", "sc_ufix", "sc_fix_fast", "sc_ufix_fast",
):
    getattr(cppyy.gbl.sc_dt, _name).__str__ = _stringify
cppyy.py.add_pythonization(_wrap_numeric_text, "sc_dt")
