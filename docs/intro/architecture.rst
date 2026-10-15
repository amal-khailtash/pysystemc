Architecture and baseline
=========================

Scope
-----

The distribution is ``pysystemc`` and the import namespace is
``amal.eda.systemc``. The supported baseline is Linux x86-64, Python 3.14,
C++17 and SystemC 2.3.4 with pthread coroutines. cppyy instantiates native
classes and templates at runtime; the project does not implement a scheduler.
A private runtime module owns dependency discovery and library loading. The
licensed SystemC entry adapter ships with the Python package, so kernel startup
does not require reading an external SystemC source tree.

Recorded local baseline
-----------------------

* Python 3.14.0; interpreter built with GCC 10.2.0.
* Native C++ compiler GCC 9.1.0.
* cppyy 3.5.0, CPyCppyy 1.13.0, cppyy-backend 1.15.3,
  cppyy-cling 6.32.8.
* SystemC commit ``5fc1469339775b54b1fcc2020ac744a58be5b50e``;
  C++17 and ``ENABLE_PTHREADS=TRUE`` in both native build configurations.
* fmt 11.2.0 and spdlog 1.15.3.

Published Linux wheels are built in a manylinux container, repaired with
auditwheel, and checked outside the checkout. Source distributions download
and compile the same pinned native dependencies when constructing a wheel.
The release workflow verifies a runtime-compiled native thread and timed wait.
This is one baseline, not a compiler/platform compatibility matrix.

Ownership and scheduling
------------------------

A process has one native kernel context. Independent simulations run in fresh
subprocesses. Python retains callbacks on their module proxies; keep modules,
signals, channels and referenced buffers alive until native processes finish.
Blocking waits release the GIL; cppyy acquires it when entering Python callbacks.
Keep ``sc_module_name`` alive through each Python module constructor to preserve
elaboration hierarchy. Native templates retain arbitrary parameter values.

Known blockers
--------------

* Reset/unwind: cppyy consumes native scheduler unwind exceptions. Python reset
  configuration is rejected explicitly instead of entering the fatal path.
* Export conversion: use ``get_interface()`` for port/export interface binding.
* TLM transport callbacks and custom Python interfaces require further validation.
* Broad tracing, clocked-thread, resolved-signal and upstream example matrices
  remain incomplete.

See :doc:`supported` for verified behavior. Availability through namespace
reflection does not establish that every native operation is safe from Python.

Python module registration
--------------------------

``sc_core.sc_module`` is the Python modeling alias for the callback-capable
native ``ScModule`` adapter. ``cppyy.gbl.sc_core.sc_module`` remains the original
SystemC class, so C++ reflection and mixed-language models retain native identity.

Keep each Python module proxy alive until all its processes finish and lifecycle
callbacks have run. Store ports, signals, event finders and channels on the module
or another owner with the same lifetime. Registration retains callable objects
only after native registration succeeds; non-callables raise ``TypeError``.
Garbage collection of temporary Python references does not invalidate registered
callbacks while the module owner is retained. This is an explicit ownership
contract, not support for deleting live modules during simulation.

Method callbacks remain retained after ``sc_stop()``: stopping the kernel does
not destroy its processes or their module hosts. Dynamic callbacks are likewise
retained for the interpreter's simulation lifetime. Do not clear callback lists
to reclaim memory while native hosts remain alive. Independent repeated runs
use new subprocesses so kernel and callback ownership end together.
Normal interpreter shutdown is checked with multiple module owners, bound-method
cycles and standalone callables: all owners and callbacks are destroyed after
their verified process execution. Callback destructors can run late in Python
shutdown, when module globals and builtins have been cleared; user cleanup code
must retain any resources it needs rather than importing or looking up globals.

The Python entry bridge borrows ``__main__`` and owns its temporary callable,
argument list and result only for the call. It releases these references and
the acquired GIL on success and Python errors. Arguments use Python filesystem
decoding; returned statuses must fit a native ``int``. Conversion errors print
their Python diagnostic and return ``-1``.
``SystemExit`` and ``KeyboardInterrupt`` raised inside ``sc_main`` follow the
same failure path; they do not bypass bridge cleanup or produce a successful
entry status. Scoped ownership releases references and GIL state during native
C++ exception unwinding as well. Allocation failure paths share this cleanup,
although forced out-of-memory behavior has not been validated.

The native process adapter allocates callback storage before creating a kernel
process. Rejected argument conversions add no process and retain no Python
callback. This does not promise rollback for allocation failures inside the
SystemC kernel itself. Static sensitivity to interfaces, ports, explicit events
and positive-edge finders is compared with a native C++ reference, including
``dont_initialize()`` and bounded simulation starts. Python errors in method
callbacks and all four lifecycle hooks are checked in isolated subprocesses.
