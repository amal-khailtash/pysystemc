TLM values and payload queues
=============================

The ``tlm`` namespace exposes native command, response, phase, synchronization,
payload-option, socket-category and endianness enums. Version functions and
metadata describe the loaded native TLM implementation. Host endianness helpers
describe the machine running the simulation.

Get and peek interfaces
-----------------------

TLM get/peek interfaces preserve arbitrary native value types and widths.
``peek(output)`` selects the native blocking-peek output overload across combined
interfaces, preserving the channel's virtual peek implementation and leaving
the sample available for a subsequent get. Output storage remains caller-owned
through a blocking operation. Native conversion and callback errors propagate.

Direct memory access
--------------------

``tlm.tlm_dmi`` stores a borrowed byte pointer, inclusive 64-bit address range,
read/write latency and access permissions. Keep its backing buffer alive while
any initiator can use the grant. Permission setters replace the previous grant;
``init()`` restores the native empty grant and clears the pointer. Address and
latency setters preserve native values without narrowing to Python word size.

Phases and temporal decoupling
------------------------------

``tlm.tlm_phase`` accepts standard phase enums and native numeric identifiers.
Registered native extended phases retain their names and stream representation.
``tlm.tlm_global_quantum.instance()`` refers to the process-wide native singleton.
Set the global quantum before resetting a keeper's local synchronization boundary.

``tlm_utils.tlm_quantumkeeper`` tracks local time and synchronizes through native
SystemC waits. Call ``sync()`` and ``set_and_sync()`` from a thread process.
They release the GIL during suspension and preserve Python cleanup when the
process is reset or killed. Keep the keeper alive through suspension.

Payload event queues
--------------------

``tlm_utils.peq_with_get[Payload]`` accepts arbitrary native payload types.
``transaction_type`` names that payload type; ``pair_type`` is the native
``std::pair<const sc_core::sc_time, Payload*>`` alias. The pair owns its time
value and borrows its payload pointer.

Notifications borrow payloads until delivery or cancellation. Retain each payload
and any data it references, wait on ``get_event()``, and drain
``get_next_transaction()`` until it returns a null pointer. ``cancel_all()``
removes pending notifications without taking ownership of their payloads.

``tlm_utils.callback_peq(name, callback, protocol=None)`` delivers a borrowed
payload and copied phase to a retained Python callback. Custom native protocol
types preserve their payload and phase types. Callbacks run with the GIL and
must not wait. Native notification timing determines immediate, delta and timed
delivery.

Native FIFOs and channels
-------------------------

``tlm.tlm_fifo[Value]`` preserves native bounded/unbounded queue behavior,
nonblocking reads and writes, debug peeks/pokes and capacity configuration.
Blocking ``get()``, ``peek()`` and ``put(value)`` release the GIL during native
waits. Get and peek also accept caller-owned mutable output storage or a native
``tlm_tag`` pointer for the value-returning overload. Keep the channel and
argument storage alive throughout suspension. Reset and kill run Python
``finally`` cleanup before the scheduler resumes native exception handling.
Nonblocking operations preserve native delta-cycle update semantics.

``tlm.tlm_analysis_fifo[Value]`` returns the original native specialization,
prepared on demand to repair dependent-base lookup in SystemC 2.3.4's two
``write`` definitions. Both direct values and analysis triples enqueue copied
transactions. Pointer payloads copy pointers and borrow their pointees. Native
custom value types and arbitrary integer widths remain available. Resolve the
template through this namespace before directly instantiating the same raw
cppyy specialization; C++ cannot specialize a method after instantiation.
Repeated lookups of equivalent native types return the same class proxy.

Analysis ports broadcast writes to their bound native interfaces; unbinding
removes a destination without deleting it. Nonblocking get/peek/put ports and
native event finders preserve their interface readiness events and sensitivity.
Retain interface owners and finders for their required native lifetimes.

Request/response and transport channels expose native exports and internal
queues. ``tlm_transport_to_master`` forwards blocking requests and responses
through its bound master port. In SystemC 2.3.4, ``tlm_slave_to_transport``
constructors do not register the private ``run`` thread: binding its ports alone
does not consume requests. The binding preserves this native behavior.
