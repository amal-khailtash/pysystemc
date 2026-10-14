Setup and validation
====================

Load Python 3.14 and install uv, then run::

   make setup
   make test-verbose
   make coverage
   make docs

Setup synchronizes dependencies, repairs CPyCppyy headers, fetches fmt and
builds pinned SystemC 2.3.4 with pthreads. Both native builders share the default
installation prefix, which the runtime discovers automatically. Existing
``UV_*`` settings are honored. Each simulation runs in a separate process.

``make setup-systemc`` selects Autotools; ``make setup-systemc-cmake`` selects
CMake. fmt is header-only. The bridge is JIT compiled and still requires upstream
kernel entry source; clean external package installation remains a release gate.

Documentation version comes from ``pyproject.toml``. Keep notable changes under
``Unreleased`` in ``CHANGELOG.md`` until an actual release is prepared.
