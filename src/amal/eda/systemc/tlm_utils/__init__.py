"""Native TLM convenience sockets, payload queues and temporal decoupling.

Templates retain arbitrary native parameters. Queues borrow payloads; keep them
alive until delivery or cancellation. Socket callback registration and utility
ownership require native-compatible objects and retained Python callbacks.
"""

import cppyy
from pathlib import Path
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
cppyy.include(str(Path(__file__).with_name("callback_peq.h")))
cppyy.include(str(Path(__file__).with_name("socket_callbacks.h")))
_callback_queues = []
_callback_sockets = []


def callback_target_socket(name, *, width=32, protocol=None, policy=None):
    """Create a native simple target socket accepting blocking Python callbacks.

    Args:
        name: Native socket name during elaboration.
        width: Arbitrary native bus width.
        protocol: Native protocol types; defaults to base protocol.
        policy: Native port policy; defaults to one or more bound.

    Returns:
        Native socket with register_b_transport(callable), bind and native accessors.

    Retain socket until simulation ends. Registered callbacks are retained until
    shutdown. Payload/delay references are borrowed; guarded waits and errors
    follow native process semantics. Nonblocking/debug/DMI registration is pending.
    """
    if protocol is None:
        protocol = cppyy.gbl.tlm.tlm_base_protocol_types
    if policy is None:
        policy = cppyy.gbl.sc_core.SC_ONE_OR_MORE_BOUND
    socket_type = cppyy.gbl.pysystemc_interfaces.CallbackTargetSocket[width, protocol, policy]
    native_register = socket_type.register_b_transport
    if not getattr(socket_type, "_retained_registration", False):

        def register(socket, callback):
            """Retain a callable after native registration succeeds."""
            if not callable(callback):
                raise TypeError("blocking transport callback must be callable")
            native_register(socket, callback)
            _callback_sockets.append((socket, callback))

        socket_type.register_b_transport = register
        socket_type._retained_registration = True
    return socket_type(name)


def callback_peq(name, callback, protocol=None):
    """Create a native payload queue delivering to a Python callable.

    Args:
        name: Native object name, created before simulation starts.
        callback: Callable accepting borrowed payload and const phase; must not wait.
        protocol: Native protocol types class; defaults to tlm_base_protocol_types.

    Returns:
        Native queue with notify(payload, phase[, delay]) and cancel_all().

    Raises:
        TypeError: Callback is not callable or protocol signatures are incompatible.

    Queue and callback are retained until interpreter shutdown because the kernel
    owns a process borrowing the queue. Retain payloads until delivery/cancellation.
    Callback exceptions propagate through the kernel. cppyy acquires the GIL.
    """
    if not callable(callback):
        raise TypeError("payload queue callback must be callable")
    if protocol is None:
        protocol = cppyy.gbl.tlm.tlm_base_protocol_types
    queue = cppyy.gbl.pysystemc_interfaces.CallbackPeq[protocol](name, callback)
    _callback_queues.append((queue, callback))
    return queue


__all__.append("callback_peq")
__all__.append("callback_target_socket")


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
