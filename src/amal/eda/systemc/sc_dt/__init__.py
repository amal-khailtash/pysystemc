"""Native SystemC integers, logic vectors and fixed-point datatypes.

Templates are instantiated by cppyy on demand. Fixed-point quantization and
overflow policies are the native sc_dt enums, not Python substitutes.
"""

import cppyy

from .._namespace import (
    is_public_name as _is_public_name,
    presented_names as _presented_names,
    document_member as _document_member,
    namespace_help as _namespace_help,
)

from . import _native_setup  # noqa: F401
from . import _bit  # noqa: F401
from . import _operators  # noqa: F401
from . import _ranges  # noqa: F401
from . import _integers  # noqa: F401
from . import _integer_owners  # noqa: F401
from . import _vectors  # noqa: F401
from . import _strings  # noqa: F401
from . import _streams  # noqa: F401
from . import _concat
from . import _shifts  # noqa: F401
from . import _numeric_operators
from . import _fixed_constructors  # noqa: F401
from . import _fxnum_owners

_namespace = cppyy.gbl.sc_dt
concat = _concat.concat
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


def _fixed_bit_member(number_type):
    """Select the native mutable overload for fixed-point bit access."""
    native = number_type.bit
    operation = cppyy.gbl.pysystemc_datatypes.fixed_number_bits[number_type].bit

    def bit(number, index):
        """Return a mutable native bit proxy and retain its parent number.

        Args:
            number: Live native fixed-point owner retained by the returned view.
            index: Native storage-bit index in ``[0, number.wl())``; the native
                fractional-word length determines the represented exponent.
        Returns:
            A native mutable bit proxy borrowing the retained parent number.
            Further native copies of proxies retain native lifetime rules.
        Raises:
            TypeError: The index cannot convert to native int.
            Native exception: A configured bounds report throws.
        Calls hold the GIL and do not suspend. Native report actions can
        terminate the process. The parent's value changes through this proxy.
        """
        result = operation(number, index)
        result._pysystemc_parent = number
        return result

    bit.__overload__ = native.__overload__
    number_type.bit = bit


for _number_type in (_namespace.sc_fxnum, _namespace.sc_fxnum_fast):
    _fixed_bit_member(_number_type)


def _fixed_range_member(number_type, name):
    """Select exact mutable native range and call operators."""
    native = getattr(number_type, name)
    operation = cppyy.gbl.pysystemc_datatypes.fixed_number_bits[number_type].range

    def range_view(number, *indices):
        """Return a mutable native range view retaining its parent number.

        Args:
            number: Live fixed-point owner retained by the returned view.
            indices: Inclusive high/low storage-bit indices, or no arguments
                for the whole configured word. Native reversed ranges apply.
        Returns:
            A mutable native subreference borrowing the retained number.
        Raises:
            TypeError: Arguments do not match a native range overload.
            Native exception: A configured native bounds report throws.
        Calls hold the GIL and never suspend. Native copies of the returned
        proxy require caller-retained parents and preserve native view semantics.
        """
        result = operation(number, *indices)
        result._pysystemc_parent = number
        return result

    range_view.__overload__ = native.__overload__
    setattr(number_type, name, range_view)


for _number_type in (_namespace.sc_fxnum, _namespace.sc_fxnum_fast):
    for _range_name in ("range", "__call__"):
        _fixed_range_member(_number_type, _range_name)


sc_fxnum, sc_fxnum_fast = _fxnum_owners.install()

for _function_name in ("add", "sub", "mult", "div"):
    globals()[_function_name] = _numeric_operators.named_function(_function_name)

_PYTHON_ADAPTERS = (
    "concat",
    "sc_context",
    "sc_fxtype_context",
    "sc_fxcast_context",
    "sc_length_context",
    "to_string",
    "sc_fxnum",
    "sc_fxnum_fast",
    "add",
    "sub",
    "mult",
    "div",
)

__all__ = _presented_names(globals(), _namespace, adapters=_PYTHON_ADAPTERS)


def __getattr__(name):
    """Resolve and cache a native datatype, template, enum or function.

    Raises AttributeError for names absent from the native sc_dt namespace.
    """
    if not _is_public_name(_namespace, name):
        raise AttributeError(name)
    member = _document_member(getattr(_namespace, name))
    globals()[name] = member
    return member


def __dir__():
    """List Python exports and discoverable native sc_dt members."""
    module_names = {name for name in globals() if name.startswith("_") or _is_public_name(_namespace, name)}
    return sorted(module_names | set(_presented_names(globals(), _namespace, adapters=_PYTHON_ADAPTERS)))


__doc__ = _namespace_help(globals(), _namespace)
