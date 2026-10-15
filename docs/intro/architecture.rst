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
