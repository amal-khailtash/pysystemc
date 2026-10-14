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
   arguments select native timed/event overloads. Reset/unwind is unsupported.

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
