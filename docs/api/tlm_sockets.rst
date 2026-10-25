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
The native const ``get_base_interface()`` overloads on multi sockets deliberately
report an error; use the mutable overload for interface access. Const export
access retains the upstream implementation's hierarchy behavior.
Mutable multi-initiator export access can bind the native dummy interface. Calling
it again on an export that is already bound reports the native binding error;
use the hierarchy-disabled socket or the const overload to inspect that export.

Payloads, phases, delays and DMI outputs are borrowed native objects. Keep payloads
and any referenced data buffers alive throughout transport and asynchronous
responses. Registration retains sockets and Python callback owners until
interpreter shutdown. Earlier teardown and owner release are still unverified.
cppyy acquires the GIL on Python callback entry. Only blocking callbacks may use
guarded waits; nonblocking, debug, DMI and invalidation callbacks must not wait.
Callback errors propagate. Tested blocking reset/kill paths execute Python cleanup
before native scheduler unwind.

Native member-pointer callbacks
-------------------------------

Native ``multi_passthrough_initiator_socket[MODULE, ...]`` and
``multi_passthrough_target_socket[MODULE, ...]`` retain the native registration
signature ``(owner, member_pointer)``. Their callback typedefs are exposed as
native value holders because cppyy cannot reflect C++ member-pointer types
directly. For example, with a native ``Owner::transport`` member::

   Socket = tlm_utils.multi_passthrough_target_socket[Owner, 73]
   callback = Socket.b_cb.from_member("Owner::transport")
   socket = Socket("target")
   socket.register_b_transport(owner, callback)

``from_member`` requires a qualified native member name without a leading ``&``.
C++ checks the exact callback signature; incompatible signatures raise cppyy
template conversion errors. Holders support default null construction, copying
and ``valid()``. A null pointer must not be used for transport dispatch. Initiator
aliases are ``nb_cb`` and ``dmi_cb``; target aliases are ``nb_cb``, ``b_cb``,
``dbg_cb`` and ``dmi_cb``. These values store the native pointer and do not own
the callback module. Keep ``owner`` alive until the socket is no longer used.
Registration runs with the GIL held and does not invoke the callback.

Verification scope
------------------

Regression models under ``tests/models/tlm_*`` check connected callback state,
connection IDs, timing, error propagation and selected reset/kill paths. Custom
payload probes preserve 137-bit native fields and transaction identity. Simple
and passthrough custom probes have independent C++ references.
``tests/models/tlm_socket_wrappers_public.py`` checks default and named
constructors for simple, tagged, passthrough and optional multi wrappers with
native custom protocol types and a 73-bit bus width. Required sockets elaborate
with bindings; optional wrappers permit unbound ports subject to their native
export requirements. Selected connected paths compare payload values, phases
and callback times with a native reference. Wrapper construction coverage does
not establish complete inherited socket overload or callback lifetime support.

The ``tlm_utils_simple_b_public``, ``tlm_utils_passthrough_b_public`` and
``tlm_utils_multi_public`` models cover the declared socket base and multi-socket
families. Native MODULE callbacks exercise all registration signatures; both
multi hierarchy binding forms resolve two indexed connections. The models check
native protocol aliases, constructor defaults, protected lifecycle hooks,
forward/backward values and phases, debug counts, DMI fields, invalidations and
owner destruction. Each has an independent C++ reference. Exact const accessors
retain native restrictions, and borrowed proxies are released before teardown.

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
