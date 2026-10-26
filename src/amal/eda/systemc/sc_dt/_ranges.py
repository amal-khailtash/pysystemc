"""Preserve native fixed-point range conversions and deprecated aliases."""

from pathlib import Path
import cppyy

cppyy.include(str(Path(__file__).with_name("ranges.h")))


def _binary_vector_constructor(native):
    """Select native fixed-range conversion before constructing a binary vector."""

    def construct(owner, *args):
        """Initialize independent vector storage from a native fixed range.

        Args:
            args: Native constructor arguments. A single fixed-point range
                uses its exact native binary-vector conversion operator.
        Returns:
            None, matching native construction.
        Raises:
            TypeError: Native construction/conversion is unavailable.
            Native exception: Allocation or a configured report throws.
        Calls hold the GIL and do not suspend. The source is borrowed only
        during construction; the resulting vector owns copied native storage.
        """
        if len(args) == 1 and isinstance(
            args[0],
            (
                cppyy.gbl.sc_dt.sc_fxnum_subref_r,
                cppyy.gbl.sc_dt.sc_fxnum_fast_subref_r,
            ),
        ):
            value = cppyy.gbl.pysystemc_datatypes.range_to_vector[type(args[0])](args[0])
            return native(owner, value)
        return native(owner, *args)

    construct.__overload__ = native.__overload__
    return construct


def _vector_pythonization(native_type, name):
    """Adapt arbitrary-width binary vector constructors without changing types."""
    if name == "sc_bv_base" or name.startswith("sc_bv<"):
        native_type.__init__ = _binary_vector_constructor(native_type.__init__)


_vector_pythonization(cppyy.gbl.sc_dt.sc_bv_base, "sc_bv_base")
cppyy.py.add_pythonization(_vector_pythonization, "sc_dt")


def _legacy_signed(view):
    """Return native signed conversion using the deprecated alias name.

    Args:
        view: Native integer owner or fixed-point range borrowing a live parent.
    Returns:
        int: Native ``to_int()`` result, identical to native ``to_signed()``.
    Raises:
        Native exception: A configured report throws.
    Keep the owner or the view and its parent live. Calls hold the GIL and
    never suspend or retain additional references.
    """
    return view.to_int()


def _legacy_unsigned(view):
    """Return native unsigned conversion using the deprecated alias name.

    Args:
        view: Native integer owner or fixed-point range borrowing a live parent.
    Returns:
        int: Native ``to_uint()`` result, identical to native ``to_unsigned()``.
    Raises:
        Native exception: A configured report throws.
    Keep the owner or the view and its parent live. Calls hold the GIL and
    never suspend or retain additional references.
    """
    return view.to_uint()


for _type in (
    cppyy.gbl.sc_dt.sc_fxnum_subref_r,
    cppyy.gbl.sc_dt.sc_fxnum_fast_subref_r,
    cppyy.gbl.sc_dt.sc_signed,
    cppyy.gbl.sc_dt.sc_unsigned,
):
    if not hasattr(_type, "to_signed"):
        _type.to_signed = _legacy_signed
    if not hasattr(_type, "to_unsigned"):
        _type.to_unsigned = _legacy_unsigned


def _legacy_bitwidth(view):
    """Return native view length through the deprecated bitwidth alias.

    Args:
        view: Integer bit, range or concatenation reference borrowing live parents.
    Returns:
        int: Native ``length()`` result, identical to native ``bitwidth()``.
    Raises:
        Native exception: A configured report throws.
    Calls hold the GIL and do not suspend or retain any borrowed storage.
    """
    return view.length()


for _name in ("sc_int_bitref_r", "sc_uint_bitref_r", "sc_int_subref_r", "sc_uint_subref_r", "sc_concatref"):
    _type = getattr(cppyy.gbl.sc_dt, _name)
    if not hasattr(_type, "bitwidth"):
        _type.bitwidth = _legacy_bitwidth
