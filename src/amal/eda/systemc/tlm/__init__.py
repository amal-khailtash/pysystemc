"""Native TLM payloads, interfaces, sockets, phases and protocol enums.

Objects retain their native ownership semantics. A generic payload does not own
its data or byte-enable buffers: keep native storage alive for every transaction.
Namespace availability does not establish Python transport callback support.
"""

import cppyy
from . import _interfaces  # noqa: F401
from . import _fifo  # noqa: F401
from ._analysis import tlm_analysis_fifo as tlm_analysis_fifo
from ._endian import tlm_to_hostendian_generic as tlm_to_hostendian_generic
from .._namespace import (
    is_public_name as _is_public_name,
    presented_names as _presented_names,
    document_member as _document_member,
    namespace_help as _namespace_help,
)

_namespace = cppyy.gbl.tlm
_PYTHON_ADAPTERS = ("tlm_analysis_fifo", "tlm_to_hostendian_generic")
__all__ = _presented_names(globals(), _namespace, adapters=_PYTHON_ADAPTERS)


def __getattr__(name):
    """Resolve a native TLM name, raising AttributeError if unavailable."""
    if not _is_public_name(_namespace, name):
        raise AttributeError(name)
    member = _document_member(getattr(_namespace, name))
    globals()[name] = member
    return member


def __dir__():
    """List Python exports and discoverable native TLM members."""
    module_names = {name for name in globals() if name.startswith("_") or _is_public_name(_namespace, name)}
    return sorted(module_names | set(_presented_names(globals(), _namespace, adapters=_PYTHON_ADAPTERS)))


__doc__ = _namespace_help(globals(), _namespace)
