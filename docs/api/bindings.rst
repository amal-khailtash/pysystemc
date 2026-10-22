Binding adapters
================

Templates remain native cppyy templates, instantiated on demand. Native
pass-through availability does not by itself establish tested support.

``sc_module.sc_method(name, callback)`` / ``sc_thread(name, callback)``
   Register a no-argument Python callable, retained on the module proxy.

``sc_module.sensitive(interface_or_port_or_event)``
   Add static native sensitivity, including edge finders.

``sc_spawn(callback, name='', options=None)``
   Spawn a retained Python callable and return a native process handle. Python
   closures can retain results; callback return values are ignored.

``wait(*args)``
   Suspend the native thread with GIL release. No arguments selects a cycle wait;
   arguments select guarded native timed/event overloads, including OR/AND
   expressions. Reset/kill unwind executes Python cleanup before the original
   scheduler exception is restored. Do not suppress unwind or wait in cleanup.

``set_report_handler(callback=None)``
   Install a retained callback receiving a native report and integer action mask;
   no callback restores the native default handler.

``sc_report``
   Copy a borrowed callback report before retaining it. Copies own their message,
   file, process name and timestamp. ``__assign__(source)`` copies fields into
   an existing report; ``swap(other)`` exchanges them. ``valid()`` reflects native
   process-name context and can be false for populated reports outside a process.
   Legacy static methods ``register_id``, ``get_message``, ``is_suppressed``,
   ``suppress_id``, ``suppress_infos``, ``suppress_warnings`` and
   ``make_warnings_errors`` preserve native diagnostics and policy. These settings
   persist in the singleton kernel; restore them when finished. Destruction
   releases copy-owned fields. Borrowed views must not outlive their report.

``sc_report_handler.stop_after(subject, ...)``
   Select severity-wide, message-wide or message/severity limits using native
   severity enums. An omitted limit is -1 and removes that selected limit.
   This adapter avoids treating a severity enum as an integer message limit.

``sc_report_handler.get_handler()`` / ``set_handler(token_or_none)``
   Save and restore borrowed native handler pointer tokens. ``None`` restores
   the native default. Tokens do not retain Python callbacks; use
   ``set_report_handler`` for Python callables. A saved Python dispatcher token
   requires its callback to remain retained and active before reinstalling it.
   Report caches are borrowed and invalidate on replacement, clear or release;
   copy reports first. Destroy copies before handler ``release()`` because message
   definitions remain owned by native handler storage.

``sc_attr_base`` / ``sc_attribute[T]`` / ``sc_attr_cltn``
   Attribute templates remain native and accept any native-valid default/copy
   constructible type. Attributes own their names and values. Collections borrow
   pointers; collection copies share those pointers. Keep attributes alive while
   either collection uses them. Removing an attribute or clearing/destroying a
   collection does not delete attributes. Native removal swaps the final pointer
   into the removed slot, so iteration order can change. Lookup returns a borrowed
   pointer or a null proxy; iterators invalidate when the collection is modified.

``sc_trace(file, signal, name)``
   Record stable native signal storage. Keep signals alive until file closure.

``sc_status()``
   Query the native current status and convert it to ``ScStatus``.

``sc_argc()`` / ``sc_argv()``
   During ``sc_main``, read the native argument count and an immutable tuple of
   original argument strings, including argument zero. The tuple copies native
   storage and survives entry return. Mutating the Python ``sc_main`` argument
   list does not change these original values. ``sc_argv()`` raises
   ``RuntimeError`` outside active entry because native argument storage is
   borrowed only during entry. Native C++ retains its pointer-array signature.

``sc_elab_and_sim(argc=None, argv=None)``
   Enter ``sc_main`` with ``sys.argv`` when both arguments are omitted, or pass
   an explicit nonnegative native count and a list/tuple of strings. Only the
   first ``argc`` entries are selected. Invalid counts, incomplete arguments,
   nonstring selected entries and embedded NULs fail before native entry.
   The native integer status is returned, including nonzero results. Use a
   fresh process for each independent simulation; entry does not reset the kernel.

Diagnostics
-----------

``PYSYSTEMC_LOG_LEVEL=DEBUG`` enables binding diagnostics. Color follows
``PYSYSTEMC_COLOR=auto/always/never``. Explicit ``always`` overrides ``NO_COLOR``;
automatic mode honors it. Native messages use fmt; Python messages use Loguru
and Rich without changing application logging sinks.

The package exposes ``__version__`` from installed metadata and prints it before
the native kernel banner. ``PYSC_COPYRIGHT_MESSAGE=DISABLE`` suppresses only the
binding banner.

Native namespaces and enums
---------------------------

``sc_core`` exposes native time/status, port/writer policies, events, channels,
exports and process classes alongside the Python callback adapters. ``sc_dt``
exposes integer, logic/vector and fixed-point templates and native policy enums.
``dir(namespace)`` and wildcard imports include resolvable native names. Templates
remain parameterized at runtime; there is no registry of permitted widths.

The package initializer selects C++17 before importing cppyy to match the native
SystemC build. Submodules inherit this setting. Existing ``EXTRA_CLING_ARGS``
values are preserved.

Fixed-point headers are enabled before SystemC is parsed. For example::

   from amal.eda.systemc import sc_dt
   value = sc_dt.sc_fixed[8, 4, sc_dt.SC_TRN, sc_dt.SC_SAT](20)
   assert value.to_double() == 7.9375

The ``tlm`` module exposes native TLM protocol enums, generic payloads, phases,
interfaces and socket templates::

   from amal.eda.systemc import tlm
   payload = tlm.tlm_generic_payload()
   payload.set_command(tlm.TLM_WRITE_COMMAND)
   payload.set_address(0x1000)
   payload.set_response_status(tlm.TLM_OK_RESPONSE)

Generic payloads do not own their data or byte-enable buffers. Retain native
buffer storage throughout the transaction. Payload fields and fixed-point
policies are tested. Callable socket factories and their verification limits are
described in :doc:`tlm_sockets`. Full transport and custom-interface coverage
remains incomplete.

Interactive help
----------------

Use standard Python introspection to explore the included native API::

   from amal.eda.systemc import sc_core, sc_dt, tlm
   help(sc_dt)
   help(sc_dt.sc_q_mode)
   help(sc_dt.sc_uint)
   help(sc_dt.sc_uint[17])
   help(tlm.tlm_generic_payload.set_address)
   dir(sc_core)

Namespace help groups classes, templates, enums, functions and constants, and
lists enum values. Native class help retains cppyy's method and constructor
signatures; templates describe parameterized instantiation. Enum type metadata
is read-only in cppyy, so enum help uses its native members while namespace help
provides the formatted values. Newly resolved class/template proxies receive
missing documentation without replacing existing native overload help.

Trace lifecycle
---------------

``sc_core.vcd_trace(filename, time_unit=None)`` closes a native VCD trace when
its context exits, including Python exceptions. Keep all traced objects alive
until that point::

   with sc_core.vcd_trace("waves", time_unit=(1, sc_core.SC_NS)) as trace:
       sc_core.sc_trace(trace, signal, "signal")
       sc_core.sc_start(10, sc_core.SC_NS)

SystemC appends ``.vcd`` to the filename. Exceptions propagate after closure;
native aborts cannot run Python cleanup. See ``examples/toy/trace_cleanup.py``.

Hierarchical interfaces
-----------------------

``examples/sysc/2.1/sc_export/main.py`` binds native custom interfaces through
Python module exports and ports. Keep ``sc_module_name`` alive throughout each
module constructor. Use ``export.get_interface()`` when binding a port or another
export: cppyy does not provide SystemC's implicit export/interface conversion.

Runtime initialization
----------------------

Native library loading and CPyCppyy header repair are centralized in the private
runtime module. The package ships its licensed native entry adapter and requires
no external kernel source file at startup. Existing environment overrides remain
available; ordinary installed wheels discover bundled dependencies automatically.

Structural reference results
----------------------------

Explicit ``sc.interface(methods=...)`` declarations support lvalue reference
results such as ``const sc_dt::sc_biguint<137>&`` and ``sc_core::sc_event&``.
The ordinary Python method returns a cppyy native object. A native pointer
trampoline preserves its address rather than copying the value. The structural
adapter retains each distinct returned object through interpreter shutdown, so
previously borrowed references survive replacement of the owner's storage.
Return long-lived storage: creating a fresh object on every call retains all
those objects. Python scalars are rejected for reference results.

Mutable results preserve writable native identity; event results preserve
sensitivity to the original event. Named scalar arguments are supported.
Defaults, template-valued argument declarations, rvalue-reference results and
repeated-name overloads require a dedicated native adapter. Native signal and FIFO structural adapters are described below.
Callbacks use cppyy's GIL acquisition; blocking callers must release the GIL and
callbacks must use guarded waits. Owner teardown before shutdown remains open.


Native structural signal and FIFO interfaces
--------------------------------------------

``sc_port[sc_signal_in_if[T]]`` and ``sc_port[sc_signal_inout_if[T]]`` accept
ordinary Python owners implementing ``read``, ``get_data_ref``,
``value_changed_event`` and ``event`` (plus ``write`` for inout). Native datatype
objects returned by read/trace callbacks preserve borrowed address identity.
Fundamental C++ scalar types use stable adapter-owned read/trace slots; logic and bool owners also supply
``posedge``, ``negedge`` and their event callbacks. ``default_event`` delegates
to the owner's value-changed event. Owners must implement native update/delta
semantics; the adapter does not schedule signal writes for them.

``sc_port[sc_fifo_in_if[T]]`` accepts ``read()``, ``nb_read()`` returning a
``(bool, value)`` pair, ``num_available`` and ``data_written_event``.
``sc_fifo_out_if[T]`` accepts ``write(value)``, ``nb_write(value)``, ``num_free``
and ``data_read_event``. Native read overloads return a value or assign an output;
a failed nonblocking read preserves the output. Write callbacks receive copies.
Blocking calls release the GIL and callbacks must use guarded waits.

Events returned by different adapter specializations retain the owner's event
identity. Each specialization remains a distinct native interface/module.
Returned native objects and owners are retained through shutdown. Full scalar conversion/range/trace matrices, signal reset registration,
full reset/error matrices and pre-shutdown teardown remain open.

The separate ``sc_fifo_blocking_in_if[T]``, ``sc_fifo_blocking_out_if[T]``,
``sc_fifo_nonblocking_in_if[T]`` and ``sc_fifo_nonblocking_out_if[T]`` templates
also accept structural owners. Blocking input requires only ``read``; blocking
output requires only ``write``. Nonblocking input requires ``nb_read`` and
``data_written_event``; nonblocking output requires ``nb_write`` and
``data_read_event``. Counts belong to combined interfaces. Borrowed events,
failed-output preservation and blocking GIL/wait rules match the combined adapters.

Native FIFO ``nb_write`` callbacks must return an actual Python bool; integers,
strings and None raise ``TypeError``. Nonblocking reads require a two-item tuple
with a bool success flag and a convertible native value. Invalid results and
callback exceptions preserve the caller's read output. Blocking callback errors
propagate to the caller; after correcting owner state, later calls remain usable.


Scalar owners return Python values from ``read``; ``get_data_ref`` uses the same
native slot and does not require another owner callback. Successful reads update
that slot immediately; owners are responsible for native update/delta semantics.
Different adapter specializations have separate scalar slots even when they borrow
the same event. Invalid conversions propagate; later valid reads remain usable.

Structural synchronization and write interfaces
------------------------------------------------

``sc_port[sc_mutex_if]`` accepts owners with ``lock``, ``trylock`` and ``unlock``.
``sc_port[sc_semaphore_if]`` requires ``wait``, ``trywait``, ``post`` and
``get_value``. Blocking adapter calls release the GIL. Python callbacks use guarded
waits; callbacks delegating to a blocking native channel must also enable GIL
release on that native call. Retain channel state through simulation.

``sc_port[sc_event_queue_if]`` requires ``notify(time)``, ``cancel_all`` and
``default_event``. Both native notify overloads forward a native sc_time value.
The default event callback returns retained native event storage.

``sc_port[sc_signal_write_if[T]]`` requires ``write(value)`` and
``get_writer_policy``. Write callbacks receive a value copy. The owner provides
native update scheduling and writer policy; the adapter does not enforce a policy
independently. Full reset/error/type and teardown matrices remain open.

Synchronization callbacks return signed native 32-bit integers; Python bools,
non-integers and out-of-range values raise ``TypeError``. Signal event/edge flags
must return bool. Writer-policy callbacks return one of the native one/many/unchecked
policies. Borrowed event callbacks require a live native sc_event object; wrong
types and null event proxies raise ``TypeError`` before native pointer dispatch.
These checks do not prevent callbacks from mutating owner state before raising.

Structural TLM ``nb_put``, ``nb_can_put``, ``nb_can_get``, ``nb_can_peek`` and
``get_direct_mem_ptr`` callbacks must return Python bool values. None, integers,
strings and other truthy objects raise ``TypeError`` rather than being converted
implicitly. Later valid callbacks remain usable. Callback mutations of payloads,
DMI descriptors or owner state are not rolled back when result validation fails.

Structural TLM FIFO ``nb_expand`` and ``nb_unbound`` callbacks return None, matching
the native void interface. ``nb_reduce``, ``nb_bound`` and debug ``nb_poke`` return
Python bool. Invalid results raise ``TypeError``; default arguments remain native
(expand/reduce 1, unbound 16, poke index 0). Owner mutations are not rolled back.

Signal inout owners may implement ``get_writer_policy`` to report their native
policy. Without this callback, the adapter returns ``SC_DEFAULT_WRITER_POLICY``.
Explicit callbacks use the same policy validation as write-only interfaces.
Policy reporting does not independently enforce owner write scheduling or contention.

Signal inout ``write(value)`` callbacks receive a native value copy, like write-only
signal and FIFO adapters. Retaining or mutating that callback argument does not
alter the native caller's storage. The owner still controls its signal state and
native update/notification scheduling.

Write-only signal and native FIFO blocking/nonblocking output callbacks also
receive Python-owned native copies. Owners can retain the callback object after
the call; caller reassignment or deletion does not invalidate that object.

Structural TLM analysis ``write``, blocking/nonblocking ``put`` and debug
``nb_poke`` callbacks receive Python-owned value copies. Retained arguments remain
valid after return and do not alias caller storage. Debug index arguments are
preserved. Transaction transport callbacks continue to borrow native transactions.

FIFO debug ``used`` and ``size`` callbacks return signed native 32-bit integers.
Bool, noninteger and out-of-range results raise ``TypeError``. Negative values
remain representable for native sentinel semantics; the owner determines whether
its counts are consistent with its FIFO state.

TLM write and debug callback contracts
--------------------------------------

Structural owners may bind ``tlm_write_if[T]``, ``tlm_analysis_if[T]``,
``tlm_delayed_write_if[T]`` and ``tlm_delayed_analysis_if[T]``. Implement
``write(value)`` or ``write(value, delay)`` respectively and return ``None``.
Values are copied before Python callback entry; delay is borrowed native time
storage and must be copied if retained beyond the callback. Compatible
write/analysis views share the same adapter. Keep owners and borrowers alive
until stop and follow the structural teardown contract above.

``transport_dbg(payload)`` must return a Python integer in the unsigned native
range and no greater than ``payload.get_data_length()``. Invalid counts raise
before native return conversion. Debug callbacks must not wait or notify events.
Native enum metadata may be lost when cppyy converts a return to Python int;
structural writer-policy callbacks must explicitly return ``sc_writer_policy``
values, including when delegating to a native channel.

Event-list expression construction
----------------------------------

``sc_event_or_list(expression)`` and ``sc_event_and_list(expression)`` consume
a matching native expression once, as native C++ list construction does. The
first-party adapter resolves cppyy's missing expression conversion. The list
owns its storage and borrows its events; keep events alive through list use and
all waiting processes. Construction runs with the GIL held on the kernel thread.
Native errors propagate. Do not reuse a consumed expression or modify a list
while a process waits on it; native assignment and swap report errors while busy, preserve waiters when the
error is thrown, and permit mutation after all waiters resume. Event union
operations retain native behavior, which does not enforce the same busy check.

Calling an event expression's ``release()`` transfers its automatic native list
to the caller as a borrowed reference. Consume it immediately by copying into
the matching owning OR/AND list. That copy deletes the automatic list; discard
the borrowed proxy and never access it afterward. Release each expression once.
