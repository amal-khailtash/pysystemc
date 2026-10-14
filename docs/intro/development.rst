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

The bridge is compiled by cppyy at runtime and loads ``libsystemc.so``. Its entry
wrapper still reads upstream kernel source; an installed wheel does not bundle
SystemC. fmt and spdlog headers are also external dependencies. Use ``FMT_HOME``
and ``SPDLOG_HOME`` for existing installations.

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

Tests verify callbacks, lifecycle, sensitivity, integer wraparound and VCD
transitions. Upstream fork/join, report and barrier ports are compared with native
C++ examples. Coverage measures Python code, including subprocesses, not native
C++. Sphinx HTML is written to ``build/docs``.

Python reset configuration raises ``NotImplementedError``: cppyy currently
consumes native unwind exceptions. The reset example is a draft, and general
kill/unwind behavior is not yet verified.

Logging and startup banners
---------------------------

Python diagnostics use Loguru and Rich; native binding diagnostics use fmt.
Debug messages are opt-in and go to stderr::

   PYSYSTEMC_LOG_LEVEL=DEBUG make example-hello-py
   PYSYSTEMC_LOG_LEVEL=DEBUG PYSYSTEMC_COLOR=always make example-counter-py

C++ runner notes use spdlog with external fmt and are visible at info level.
``make setup-spdlog`` fetches its headers and is included in aggregate setup.

``PYSYSTEMC_COLOR=auto`` enables terminal colors unless ``NO_COLOR`` is set.
``PYSYSTEMC_COLOR=always`` explicitly forces colors, including with ``NO_COLOR``.
``PYSYSTEMC_COLOR=never`` disables them. Native model output and SystemC reports
keep their original form.

The binding exposes ``amal.eda.systemc.__version__`` and prints it before the
kernel banner. ``PYSC_COPYRIGHT_MESSAGE=DISABLE`` suppresses the binding banner;
``SC_COPYRIGHT_MESSAGE=DISABLE`` suppresses the native SystemC banner.
