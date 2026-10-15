"""Native TLM payloads, interfaces, sockets, phases and protocol enums.

Objects retain their native ownership semantics. A generic payload does not own
its data or byte-enable buffers: keep native storage alive for every transaction.
Namespace availability does not establish Python transport callback support.
"""
import cppyy
from .._namespace import public_names as _public_names, document_member as _document_member, namespace_help as _namespace_help

_namespace = cppyy.gbl.tlm
__all__ = _public_names(_namespace)


def __getattr__(name):
    """Resolve a native TLM name, raising AttributeError if unavailable."""
    if name.startswith('_'):
        raise AttributeError(name)
    member = _document_member(getattr(_namespace, name))
    globals()[name] = member
    return member


def __dir__():
    """List Python exports and discoverable native TLM members."""
    return sorted(set(globals()) | set(_public_names(_namespace)))

__doc__ = _namespace_help(globals(), _namespace)
