TLM socket callbacks
====================

Import callable socket factories from ``amal.eda.systemc.tlm_utils``. They wrap
native socket templates and accept Python callables for member-pointer callback
registration. ``width`` and ``protocol`` remain native template parameters;
``protocol`` defaults to ``tlm_base_protocol_types``. A custom protocol supplies
``tlm_payload_type`` and ``tlm_phase_type`` aliases.

Factory families
----------------

* ``callback_target_socket(name, width=32, protocol=None, policy=None, tagged=False)``
  creates a simple target.
* ``callback_initiator_socket(...)`` creates a simple initiator with the same
  keyword parameters.
* ``callback_passthrough_target_socket(...)`` creates a passthrough target with
  the same keyword parameters.
* ``callback_multi_target_socket(name, width=32, protocol=None, connections=0, policy=None)``
  creates a multi target.
* ``callback_multi_initiator_socket(...)`` creates a multi initiator with the
  same keyword parameters. ``connections=0`` preserves the native unlimited count.

All factory options following ``name`` are keyword-only. The default policy is
``SC_ONE_OR_MORE_BOUND``. Use ``SC_ZERO_OR_MORE_BOUND`` for optional connections.
An optional multi target still requires at least one callback registration to
bind its internal export, as required by the upstream implementation.

Registration signatures
-----------------------

Targets provide these registrations, each taking a callable::

   socket.register_b_transport(callback)          # callback(payload, delay)
   socket.register_nb_transport_fw(callback)      # callback(payload, phase, delay)
   socket.register_transport_dbg(callback)        # callback(payload)
   socket.register_get_direct_mem_ptr(callback)   # callback(payload, dmi)

Initiators provide::

   socket.register_nb_transport_bw(callback)          # callback(payload, phase, delay)
   socket.register_invalidate_direct_mem_ptr(callback) # callback(first, last)

Nonblocking transport returns a native ``tlm_sync_enum``. Debug transport returns
an integer in ``0..2**32-1``; invalid counts raise a native range exception before
unsigned conversion. DMI callbacks return a boolean and mutate the borrowed DMI
output. Blocking transport returns ``None`` and may mutate the borrowed delay.

With ``tagged=True``, registration takes ``(callback, user_id)`` and the callback
receives that ID as its first argument. Multi callbacks also receive a first ID,
but it is the native connection index and registration takes only the callable.
Forward target indices and backward initiator indices can differ on the same
multi-to-multi connection. Duplicate multi registrations warn and keep the first
callback. Passthrough duplicates also keep the first callback; tagged passthrough
registration updates its user ID even when warning about a duplicate callback.

Binding and simulation
----------------------

Create sockets inside a native module construction scope and bind before
elaboration. For example::

   from amal.eda.systemc import sc_core as sc, tlm, tlm_utils

   name = sc.sc_module_name("top")
   top = sc.sc_module(name)
   target = tlm_utils.callback_target_socket("target", width=73)
   initiator = tlm_utils.callback_initiator_socket("initiator", width=73)

   def transport(payload, delay):
       sc.wait(delay)
       delay.__assign__(sc.SC_ZERO_TIME)
       payload.set_response_status(tlm.TLM_OK_RESPONSE)

   target.register_b_transport(transport)
   initiator.bind(target)
   del name
   sc.sc_start(sc.SC_ZERO_TIME)
   interface = initiator.get_interface()

Call ``interface.b_transport(payload, delay)`` from a SystemC thread process.
Returned concrete forward interfaces configure automatic GIL release for blocking
calls. Multi initiators use ``initiator[index]`` after elaboration; multi targets
use ``target[index]`` for backward calls. Hierarchical bindings resolve during
native elaboration, so access the resolved interface after zero-time ``sc_start``.

Payloads, phases, delays and DMI outputs are borrowed native objects. Keep payloads
and any referenced data buffers alive throughout transport and asynchronous
responses. Registration retains sockets and Python callback owners until
interpreter shutdown. Earlier teardown and owner release are still unverified.
cppyy acquires the GIL on Python callback entry. Only blocking callbacks may use
guarded waits; nonblocking, debug, DMI and invalidation callbacks must not wait.
Callback errors propagate. Tested blocking reset/kill paths execute Python cleanup
before native scheduler unwind.

Verification scope
------------------

Regression models under ``tests/models/tlm_*`` check connected callback state,
connection IDs, timing, error propagation and selected reset/kill paths. Custom
payload probes preserve 137-bit native fields and transaction identity. Simple
and passthrough custom probes have independent C++ references. These checks do
not establish complete socket overload, protocol, hierarchy or lifetime coverage.
M6A remains incomplete.

Callback return contracts
~~~~~~~~~~~~~~~~~~~~~~~~~

Convenience factory registrations validate Python results before native conversion.
Blocking transport and DMI invalidation return ``None``; DMI permission returns
``bool``; debug transport returns an integer from zero through ``2**32 - 1``,
excluding bool. Forward and backward nonblocking callbacks return a native
``tlm_sync_enum`` value: ``TLM_ACCEPTED``, ``TLM_UPDATED`` or ``TLM_COMPLETED``.
Malformed types raise ``TypeError``; invalid ranges raise ``ValueError``. Payload,
phase and delay arguments remain borrowed and callback mutations precede validation.
These contracts apply to simple, tagged, passthrough and multi factories.
