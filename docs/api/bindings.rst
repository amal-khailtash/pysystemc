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
policies are tested; socket transport callbacks and custom Python interfaces
still require validation and are not yet claimed as supported.

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
