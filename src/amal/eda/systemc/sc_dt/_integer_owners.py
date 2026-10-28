"""Select exact native integer owner construction and assignment."""

import cppyy

from ._integers import _integer_text
from ._native_methods import NativeMethod


def _adapt(native_type):
    """Preserve native width construction and dispatch exact datatype sources."""
    native_constructor = native_type.__init__
    copy_constructor = native_constructor.__overload__(f"const {native_type.__cpp_name__}&")
    native_assignment = native_type.__assign__
    text_assignment = native_assignment.__overload__("const char*")

    def construct(owner, *args, **kwargs):
        """Initialize a native integer width or independently copied source.

        Args:
            args: Native width or datatype constructor arguments.
            kwargs: Native keyword arguments, forwarded to the native constructor.
        Returns:
            None, matching native construction.
        Raises:
            TypeError: The selected native constructor is unavailable.
            Native exception: Width, conversion or allocation reporting throws.
        A scalar width retains native semantics. Datatype sources are borrowed
        for a GIL-held call and copied into independent numeric storage. No
        waits, callbacks or source references persist.
        """
        if len(args) == 1 and not kwargs:
            source = args[0]
            if getattr(type(source), "__cpp_name__", "").startswith("sc_dt::"):
                value = cppyy.gbl.pysystemc_datatypes.IntegerConversion[native_type, type(source)].construct(source)
                return copy_constructor(owner, value)
        return native_constructor(owner, *args, **kwargs)

    native_type.__init__ = NativeMethod(construct, native_constructor)

    def assign(owner, source):
        """Assign a live integer owner from an exact scalar or datatype source.

        Args:
            source: Python integer or native assignment source.
        Returns:
            The original Python owner, retaining its native numeric storage.
        Raises:
            TypeError: Native conversion or assignment is unavailable.
            Native exception: Conversion, allocation or configured reports throw.
        Python integers preserve every bit through native decimal parsing.
        Other native sources select exact overloads with the GIL held. Sources
        are borrowed only during the call; no callbacks or references persist.
        """
        if type(source) is int:
            text_assignment(owner, _integer_text(source))
        elif hasattr(type(source), "__cpp_name__"):
            cppyy.gbl.pysystemc_datatypes.IntegerConversion[native_type, type(source)].assign(owner, source)
        else:
            native_assignment(owner, source)
        return owner

    native_type.__assign__ = NativeMethod(assign, native_assignment)


for _type in (
    cppyy.gbl.sc_dt.sc_signed,
    cppyy.gbl.sc_dt.sc_unsigned,
    cppyy.gbl.sc_dt.sc_int_base,
    cppyy.gbl.sc_dt.sc_uint_base,
):
    _adapt(_type)
