"""Expose owned fixed-point base facades with direct protected initialization."""

from pathlib import Path

import cppyy

from ._fixed_constructors import _source_signature
from ._integers import _integer_text
from ._native_methods import NativeMethod
from ._numeric_operators import _adapt as _adapt_numeric

cppyy.include(str(Path(__file__).with_name("fxnum_owners.h")))


def _adapt(native_type, fast=False):
    """Initialize owned native base storage directly through exact constructor types."""
    native_constructor = native_type.__init__
    observer_signature = "sc_dt::sc_fxnum_fast_observer*" if fast else "sc_dt::sc_fxnum_observer*"

    def construct(owner, *args, **kwargs):
        """Initialize configured native numeric storage and borrow its observer.

        Args:
            args: Optional source followed by type parameters, encoding,
                cast switch and optional observer.
            kwargs: Native constructor keywords, forwarded when supplied.
        Returns:
            None, matching native construction.
        Raises:
            TypeError: Required native parameters or source conversion are invalid.
            Native exception: Allocation, conversion or configured reports throw.
        Observers must remain live through number destruction. Calls hold the
        GIL and do not retain Python callbacks or source references. Native
        private default and copy construction remain unavailable; copy values
        with the source-plus-parameters constructor instead.
        """
        if kwargs or not args:
            return native_constructor(owner, *args, **kwargs)
        arguments = list(args)
        signatures = []
        if not isinstance(arguments[0], cppyy.gbl.sc_dt.sc_fxtype_params):
            if type(arguments[0]) is int and not -(1 << 31) <= arguments[0] < (1 << 31):
                arguments[0] = _integer_text(arguments[0])
            source_signature = _source_signature(arguments[0])
            if source_signature is None:
                return native_constructor(owner, *args, **kwargs)
            signatures.append(source_signature)
        signatures.extend(
            ("const sc_dt::sc_fxtype_params&", "sc_dt::sc_enc", "const sc_dt::sc_fxcast_switch&", observer_signature)
        )
        return native_constructor.__overload__(", ".join(signatures))(owner, *arguments)

    native_type.__init__ = NativeMethod(construct, native_constructor)
    _adapt_numeric(native_type)
    native_type.__doc__ = (
        "Owned facade for a native fixed-point base. Initialize with optional "
        "source, sc_fxtype_params, sc_enc, sc_fxcast_switch and optional observer. "
        "The observer is borrowed and must remain live through destruction. "
        "Native widths, precision and reports apply; calls hold the GIL. "
        "Private default/copy construction is unavailable; the source constructor "
        "copies independent storage."
    )


sc_fxnum = cppyy.gbl.pysystemc_datatypes.FxnumOwner
sc_fxnum_fast = cppyy.gbl.pysystemc_datatypes.FxnumFastOwner
_adapt(sc_fxnum)
_adapt(sc_fxnum_fast, fast=True)
