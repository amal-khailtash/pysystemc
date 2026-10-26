"""Select exact native vector template overloads for arbitrary widths."""

from pathlib import Path
import cppyy

cppyy.include(str(Path(__file__).with_name("vectors.h")))


def _adapt_vector(native_type):
    """Install exact native-source construction and assignment on a vector."""
    original = native_type.__init__
    copy = original.__overload__(f"const {native_type.__cpp_name__}&")
    assignment = native_type.__assign__

    def construct(owner, *args, **kwargs):
        """Construct independent vector storage from native sources.

        Args:
            args: Native constructor arguments, including proxy and number sources.
            kwargs: Native keyword arguments, passed through unchanged.
        Returns:
            None.
        Raises:
            TypeError: Native construction is unavailable for this source.
            Native exception: A native allocation or configured report throws.
        The GIL remains held. Sources are borrowed only for the call; no callbacks
        or references are retained. Native width and four-state semantics apply.
        """
        if len(args) == 1 and not kwargs and hasattr(type(args[0]), "__cpp_name__"):
            source = args[0]
            if isinstance(source, (cppyy.gbl.sc_dt.sc_fxnum_subref_r, cppyy.gbl.sc_dt.sc_fxnum_fast_subref_r)):
                source = cppyy.gbl.pysystemc_datatypes.range_to_vector[type(source)](source)
            result = cppyy.gbl.pysystemc_datatypes.vector_construct[native_type, type(source)](source)
            return copy(owner, result)
        return original(owner, *args, **kwargs)

    def assign(owner, source):
        """Assign a native vector source and return the live target.

        Args:
            source: Native scalar, proxy, number or ordinary assignment source.
        Returns:
            The target vector, borrowing its existing owned storage.
        Raises:
            TypeError: The native assignment is unavailable.
            Native exception: Native conversion or configured reporting throws.
        Calls hold the GIL and never suspend or retain source references.
        """
        if hasattr(type(source), "__cpp_name__"):
            return cppyy.gbl.pysystemc_datatypes.vector_assign[native_type, type(source)](owner, source)
        return assignment(owner, source)

    construct.__overload__ = original.__overload__
    assign.__overload__ = assignment.__overload__
    native_type.__init__ = construct
    native_type.__assign__ = assign


def _pythonize(native_type, name):
    """Adapt every native vector specialization as cppyy creates it."""
    if name in ("sc_bv_base", "sc_lv_base") or name.startswith(("sc_bv<", "sc_lv<")):
        _adapt_vector(native_type)


_pythonize(cppyy.gbl.sc_dt.sc_bv_base, "sc_bv_base")
_pythonize(cppyy.gbl.sc_dt.sc_lv_base, "sc_lv_base")
cppyy.py.add_pythonization(_pythonize, "sc_dt")
