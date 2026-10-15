"""Native TLM convenience sockets, payload queues and temporal decoupling.

Templates retain arbitrary native parameters. Queues borrow payloads; keep them
alive until delivery or cancellation. Socket callback registration and utility
ownership require native-compatible objects and retained Python callbacks.
"""

import cppyy
from ..sc_core import _ProcessUnwind
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


cppyy.cppdef("""
/** @brief Preserve native scheduler unwind during quantumkeeper synchronization.
 * @param keeper Borrowed keeper retained across suspension.
 * @return False if Python must unwind before the native callback dispatcher rethrows.
 * @details Release GIL before entry. Exceptions other than scheduler unwind propagate.
 */
namespace pysystemc_interfaces {
bool quantum_sync(tlm_utils::tlm_quantumkeeper& keeper)
{
    return sc_core::python_guarded_operation([&]() { keeper.sync(); });
}
/** @brief Set local time and synchronize with guarded scheduler exception handling.
 * @details Keeper/time are borrowed; release GIL. False requests Python frame unwind.
 */
bool quantum_set_and_sync(tlm_utils::tlm_quantumkeeper& keeper, const sc_core::sc_time& time)
{
    return sc_core::python_guarded_operation([&]() { keeper.set_and_sync(time); });
}
}
""")
cppyy.gbl.pysystemc_interfaces.quantum_sync.__release_gil__ = True
cppyy.gbl.pysystemc_interfaces.quantum_set_and_sync.__release_gil__ = True


def _sync(self):
    """Synchronize native local time; propagate reset/kill through Python cleanup."""
    if not cppyy.gbl.pysystemc_interfaces.quantum_sync(self):
        raise _ProcessUnwind()


def _set_and_sync(self, time):
    """Set borrowed native time and synchronize, preserving scheduler unwind."""
    if not cppyy.gbl.pysystemc_interfaces.quantum_set_and_sync(self, time):
        raise _ProcessUnwind()


_namespace.tlm_quantumkeeper.sync = _sync
_namespace.tlm_quantumkeeper.set_and_sync = _set_and_sync
