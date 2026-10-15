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


def _configure_forward_access(socket_type):
    """Release GIL on concrete forward interfaces returned by a native socket."""
    if getattr(socket_type, "_forward_gil_configured", False):
        return
    for accessor in ("get_interface", "__getitem__"):
        native_get = getattr(socket_type, accessor, None)
        if native_get is None:
            continue

        def get_interface(socket, *args, _native=native_get):
            """Return the native interface with blocking dispatch configured for waits."""
            interface = _native(socket, *args)
            if interface:
                method = getattr(type(interface), "b_transport", None)
                if method is not None and hasattr(method, "__release_gil__"):
                    method.__release_gil__ = True
            return interface

        setattr(socket_type, accessor, get_interface)
    socket_type._forward_gil_configured = True


def callback_initiator_socket(name, *, width=32, protocol=None, policy=None, tagged=False):
    """Create a native simple initiator socket accepting backward Python callbacks.

    Args:
        name: Native socket name during elaboration.
        width: Arbitrary native bus width.
        protocol: Native protocol types, defaulting to base protocol.
        policy: Native binding policy, defaulting to one or more bound.
        tagged: Use native tagged callbacks; registration also takes a user ID.

    Returns:
        Native socket with backward transport and DMI invalidation registration.

    Callbacks and socket are retained until shutdown after registration. Payload,
    phase and delay are borrowed; backward callbacks must not wait. Callback
    errors propagate and cppyy acquires the GIL on entry.
    """
    protocol = cppyy.gbl.tlm.tlm_base_protocol_types if protocol is None else protocol
    policy = cppyy.gbl.sc_core.SC_ONE_OR_MORE_BOUND if policy is None else policy
    template = (
        cppyy.gbl.pysystemc_interfaces.CallbackTaggedInitiatorSocket
        if tagged
        else cppyy.gbl.pysystemc_interfaces.CallbackInitiatorSocket
    )
    socket_type = template[width, protocol, policy]
    _configure_forward_access(socket_type)
    if not getattr(socket_type, "_retained_registration", False):
        for method in ("register_nb_transport_bw", "register_invalidate_direct_mem_ptr"):
            native_register = getattr(socket_type, method)

            def register(socket, callback, *args, _native=native_register):
                """Retain callable ownership after successful native registration."""
                if not callable(callback):
                    raise TypeError("socket callback must be callable")
                _native(socket, callback, *args)
                _callback_sockets.append((socket, callback))

            setattr(socket_type, method, register)
        socket_type._retained_registration = True
    return socket_type(name)


def callback_target_socket(name, *, width=32, protocol=None, policy=None, tagged=False):
    """Create a native simple target socket accepting Python transport callbacks.

    Args:
        name: Native socket name during elaboration.
        width: Arbitrary native bus width.
        protocol: Native protocol types; defaults to base protocol.
        policy: Native port policy; defaults to one or more bound.
        tagged: Select native tagged socket; blocking registration takes callable and ID.

    Returns:
        Native socket with register_b_transport(callable), bind and native accessors.

    Retain socket until simulation ends. Registered callbacks are retained until
    shutdown. Payload/delay references are borrowed; guarded waits and errors
    follow native process semantics. Nonblocking/debug/DMI callbacks must not wait.
    """
    if protocol is None:
        protocol = cppyy.gbl.tlm.tlm_base_protocol_types
    if policy is None:
        policy = cppyy.gbl.sc_core.SC_ONE_OR_MORE_BOUND
    template = (
        cppyy.gbl.pysystemc_interfaces.CallbackTaggedTargetSocket
        if tagged
        else cppyy.gbl.pysystemc_interfaces.CallbackTargetSocket
    )
    socket_type = template[width, protocol, policy]
    if not getattr(socket_type, "_retained_registration", False):
        methods = (
            (
                "register_b_transport",
                "register_nb_transport_fw",
                "register_transport_dbg",
                "register_get_direct_mem_ptr",
            )
            if tagged
            else (
                "register_b_transport",
                "register_nb_transport_fw",
                "register_transport_dbg",
                "register_get_direct_mem_ptr",
            )
        )
        for method in methods:
            native_register = getattr(socket_type, method)

            def register(socket, callback, *args, _native=native_register):
                """Retain a callable after native registration succeeds."""
                if not callable(callback):
                    raise TypeError("socket transport callback must be callable")
                _native(socket, callback, *args)
                _callback_sockets.append((socket, callback))

            setattr(socket_type, method, register)
        socket_type._retained_registration = True
    return socket_type(name)


def callback_multi_target_socket(name, *, width=32, protocol=None, connections=0, policy=None):
    """Create a native multi target with connection-indexed Python callbacks.

    Args:
        name: Native socket name during elaboration.
        width: Arbitrary native bus width.
        protocol: Native protocol types; defaults to base protocol.
        connections: Native maximum connection count; zero means unlimited.
        policy: Native binding policy; defaults to one or more bound.

    Returns:
        Native multi target with forward callback registration and indexed backward access.

    Callbacks receive the upstream connection index before borrowed payload arguments.
    Registered socket/callback owners remain retained until shutdown. Blocking callbacks
    may use guarded waits; other callbacks must not wait. cppyy acquires GIL on entry;
    exceptions propagate. Bind before elaboration and index only resolved connections.
    """
    protocol = cppyy.gbl.tlm.tlm_base_protocol_types if protocol is None else protocol
    policy = cppyy.gbl.sc_core.SC_ONE_OR_MORE_BOUND if policy is None else policy
    socket_type = cppyy.gbl.pysystemc_interfaces.CallbackMultiTargetSocket[width, protocol, connections, policy]
    if not getattr(socket_type, "_retained_registration", False):
        for method in (
            "register_b_transport",
            "register_nb_transport_fw",
            "register_transport_dbg",
            "register_get_direct_mem_ptr",
        ):
            native_register = getattr(socket_type, method)

            def register(socket, callback, _native=native_register):
                """Retain callable ownership after native registration succeeds."""
                if not callable(callback):
                    raise TypeError("socket callback must be callable")
                _native(socket, callback)
                _callback_sockets.append((socket, callback))

            setattr(socket_type, method, register)
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


__all__.append("callback_multi_target_socket")
__all__.append("callback_peq")
__all__.append("callback_target_socket")
__all__.append("callback_initiator_socket")


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
