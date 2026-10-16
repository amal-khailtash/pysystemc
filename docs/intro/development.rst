Development
===========

Environment and native dependencies
-----------------------------------

Use Python 3.14 and uv. ``make setup`` synchronizes dependencies, repairs
CPyCppyy headers, fetches fmt/spdlog, and builds SystemC 2.3.4 with pthreads.
Existing ``UV_*`` settings are honored.

Both ``make setup-systemc`` (Autotools) and ``make setup-systemc-cmake`` install
into ``build/deps/systemc-install``. CMake build files live in
``build/deps/systemc``. Native headers and libraries are discovered automatically.
Override ``SYSTEMC_INSTALL_DIR`` or ``SYSTEMC_LIBRARY_DIR`` for another compatible
installation. Python 3.14 thread callbacks require the tested pthread backend.

``uv build`` fetches and builds fmt, spdlog and pthreads SystemC 2.3.4, then
stages dependencies under ``build/package-native`` and bundles native libraries, headers, license notices and the entry-wrapper source
in a platform-specific wheel. cppyy still compiles the binding adapters and user
models at runtime. Source distributions perform native preparation when built
into a wheel. Building requires curl, a C++17 compiler and CMake.

Installed wheels discover their bundled dependencies automatically. ``FMT_HOME``
and ``SPDLOG_HOME`` remain available for explicit development installations.

Header repair
-------------

Some CPyCppyy distributions install lowercase ``cpycppyy`` headers while generated
bindings include ``CPyCppyy/API.h``. ``make fix-cppyy`` creates the compatibility
alias. Package initialization also repairs it when the environment is writable.
Only the corresponding Clang case-path diagnostic is suppressed.

Validation and documentation
-----------------------------

Run the regression suite in independent native simulation processes::

   make test
   make test-verbose
   make coverage
   make docs

The coverage target prints a terminal summary and writes an HTML report to
``build/coverage/html/index.html``. Open that file in a browser to inspect
coverage by module and line.

Tests verify callbacks, lifecycle, sensitivity, integer wraparound and VCD
transitions. Upstream fork/join, report and barrier ports are compared with native
C++ examples. Coverage measures Python code, including subprocesses, not native
C++. Sphinx HTML is written to ``build/docs``.

The synchronous reset port matches native C++. Guarded wait adapters preserve
reset/kill unwind while Python cleanup runs; use the Python wait adapter and
issue handle controls from running processes. See :doc:`features` for limits.

Logging and startup banners
---------------------------

Python diagnostics use Loguru and Rich; native binding diagnostics use fmt.
Debug messages are opt-in and go to stderr::

   PYSYSTEMC_LOG_LEVEL=DEBUG make example-hello-py
   PYSYSTEMC_LOG_LEVEL=DEBUG PYSYSTEMC_COLOR=always make example-counter-py

The C++ runner reports source loading and failures on standard error.
Native model output remains on its original streams.

``PYSYSTEMC_COLOR=auto`` enables terminal colors unless ``NO_COLOR`` is set.
``PYSYSTEMC_COLOR=always`` explicitly forces colors, including with ``NO_COLOR``.
``PYSYSTEMC_COLOR=never`` disables them. Native model output and SystemC reports
keep their original form.

The binding exposes ``amal.eda.systemc.__version__`` and prints it before the
kernel banner. ``PYSC_COPYRIGHT_MESSAGE=DISABLE`` suppresses the binding banner;
``SC_COPYRIGHT_MESSAGE=DISABLE`` suppresses the native SystemC banner.

Native C++ source runner
------------------------

``pysystemc-cpp`` compiles source files in the order supplied and runs their
native ``sc_main`` through the packaged startup adapter::

   uv run pysystemc-cpp -I include helper.cpp model.cpp -- --cycles 20

Each source directory is also an include directory. Arguments after ``--``
are passed unchanged; ``argv[0]`` is the first source filename. Native return
status becomes the command exit status. File, compilation and runtime errors
produce diagnostics and a failure status. Run one model per interpreter; use
another invocation for an independent kernel. The compatibility script
``examples/run_sysc_cpp.py`` uses this same implementation. No external kernel
source installation is required.
