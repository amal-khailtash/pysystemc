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

``sc_trace(file, signal, name)``
   Record stable native signal storage. Keep signals alive until file closure.

``sc_status()``
   Query the native current status and convert it to ``ScStatus``.

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
