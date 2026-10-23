"""Select native SystemC stream insertion while preserving standard overloads."""

from enum import Enum

import cppyy

_ostream = cppyy.gbl.std.ostream


def _wrap_stream(native, name=""):
    """Adapt native ostream subclasses, including classes loaded after import."""
    if not issubclass(native, _ostream) or native.__dict__.get("_fifo_stream_wrapped", False):
        return
    original = native.__lshift__

    def insert(stream, value):
        """Insert native SystemC values or invoke the stream's original overloads.

        Stream and value storage remain borrowed for the call. The original stream
        is returned for chaining and retained by that returned Python reference.
        Native I/O failures raise OSError with the native diagnostic; insertion
        does not consume FIFO samples. Other native errors propagate.
        """
        fifo_insert = getattr(value, "_fifo_stream_insert", None)
        if fifo_insert is not None and isinstance(value, cppyy.gbl.sc_core.sc_prim_channel):
            return fifo_insert(stream)
        native = cppyy.gbl.sc_core
        if isinstance(value, Enum) and type(value).__name__ == "ScStatus" and type(value).__module__ == __package__:
            value = native.sc_status(value.value)
        specialization = None
        if isinstance(value, (native.sc_time, native.sc_status)):
            specialization = type(value)
        elif isinstance(value, native.sc_signal_channel):
            specialization = native.sc_signal_channel
        else:
            specialization = next(
                (
                    base
                    for base in type(value).__mro__
                    if getattr(base, "__cpp_name__", "").startswith(("sc_core::sc_in<", "sc_core::sc_inout<"))
                ),
                None,
            )
        if specialization is not None:
            result = native.python_public_stream[specialization].insert(stream, value)
            if not result.first:
                raise OSError(str(result.second))
            return stream
        if isinstance(value, (str, cppyy.gbl.std.string)):
            result = cppyy.gbl.sc_core.python_stream_access.text(stream, value)
            if not result.first:
                raise OSError(str(result.second))
            return stream
        return original(stream, value)

    native.__lshift__ = insert
    native._fifo_stream_wrapped = True


def install_fifo_streams():
    """Enable native SystemC insertion on current and future character ostreams."""
    for stream in (_ostream, cppyy.gbl.std.ostringstream, cppyy.gbl.std.stringstream, cppyy.gbl.std.ofstream):
        _wrap_stream(stream)
    cppyy.py.add_pythonization(_wrap_stream)
