"""Native SystemC integers, logic vectors and fixed-point datatypes.

Templates are instantiated by cppyy on demand. Fixed-point quantization and
overflow policies are the native sc_dt enums, not Python substitutes.
"""

import cppyy
from .._namespace import (
    public_names as _public_names,
    document_member as _document_member,
    namespace_help as _namespace_help,
)

_namespace = cppyy.gbl.sc_dt
__all__ = _public_names(_namespace)


def __getattr__(name):
    """Resolve and cache a native datatype, template, enum or function.

    Raises AttributeError for names absent from the native sc_dt namespace.
    """
    if name.startswith("_"):
        raise AttributeError(name)
    member = _document_member(getattr(_namespace, name))
    globals()[name] = member
    return member


def __dir__():
    """List Python exports and discoverable native sc_dt members."""
    return sorted(set(globals()) | set(_public_names(_namespace)))


__doc__ = _namespace_help(globals(), _namespace)
