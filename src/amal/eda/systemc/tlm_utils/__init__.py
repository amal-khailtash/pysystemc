"""Native TLM convenience sockets, payload queues and temporal decoupling.

Templates retain arbitrary native parameters. Queues borrow payloads; keep them
alive until delivery or cancellation. Socket callback registration and utility
ownership require native-compatible objects and retained Python callbacks.
"""

import cppyy
from .._namespace import public_names as _public_names, document_member as _document_member

for _header in (
    "tlm_quantumkeeper.h",
    "peq_with_get.h",
    "peq_with_cb_and_phase.h",
    "simple_initiator_socket.h",
    "simple_target_socket.h",
    "passthrough_target_socket.h",
    "multi_passthrough_initiator_socket.h",
    "multi_passthrough_target_socket.h",
    "instance_specific_extensions.h",
):
    cppyy.include(f"tlm_utils/{_header}")

_namespace = cppyy.gbl.tlm_utils
__all__ = _public_names(_namespace)


def __getattr__(name):
    """Resolve a native utility; unavailable names raise AttributeError.

    Returned native classes preserve constructor and template signatures.
    Callers retain queues, payloads and socket callback owners through simulation.
    """
    if name.startswith("_"):
        raise AttributeError(name)
    member = _document_member(getattr(_namespace, name))
    globals()[name] = member
    return member


def __dir__():
    """List utility names available in the native namespace."""
    return sorted(set(globals()) | set(_public_names(_namespace)))


# Native sync uses a scheduler wait. Release GIL during suspension; reset/unwind
# parity still requires the separate process-control validation matrix.
_namespace.tlm_quantumkeeper.sync.__release_gil__ = True
_namespace.tlm_quantumkeeper.set_and_sync.__release_gil__ = True
