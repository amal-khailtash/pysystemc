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
   make test-parallel TEST_WORKERS=4
   make coverage-parallel TEST_WORKERS=4
   make docs

The coverage target prints a terminal summary and writes an HTML report to
``build/coverage/html/index.html``. Open that file in a browser to inspect
coverage by module and line. Parallel coverage combines worker and simulation
subprocess data; each simulation remains isolated. See :doc:`releases` for CI
validation, publication ordering and recovery.

Tests verify callbacks, lifecycle, sensitivity, integer wraparound and VCD
transitions. Upstream fork/join, report and barrier ports are compared with native
C++ examples. Coverage measures Python code, including subprocesses, not native
C++. Sphinx HTML is written to ``build/docs``.

The synchronous reset port matches native C++. Guarded wait adapters preserve
reset/kill unwind while Python cleanup runs; use the Python wait adapter and
issue handle controls from running processes. See :doc:`../intro/features` for limits.

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

Public declaration audit
~~~~~~~~~~~~~~~~~~~~~~~~

Run ``uv run python tools/inventory_public_api.py`` to extract the modeling
configuration, or add ``--configuration baseline`` to omit explicit dynamic-process
and fixed-point defines. The JSON records both requested configuration and effective
feature defines: convenience socket and PEQ headers can enable dynamic processes.
Inheritance metadata and parameter default presence help reconcile inherited APIs
and callable overloads. These records do not establish semantic support.

To retain an audit, pass ``--reviews path/to/reviews.json``. The file is a JSON
object keyed by the emitted declaration ``id``. IDs distinguish overloads and
remain unchanged when source lines move; header, signature or template changes
can require a new review. Unknown IDs fail validation rather than silently losing
reviews. A review may contain ``classification`` (unreviewed, public, internal),
``status`` (untested, supported, failing, unsupported, deferred), ``rationale``,
``python``, ``ownership``, ``gil``, ``errors`` and ``evidence``.

Classified declarations require a rationale. Supported declarations must be
public and supply nonempty Python spelling, ownership, GIL, errors and an evidence
list identifying executable checks. Review validation checks the record structure;
a reviewer must assess the cited evidence and source rationale. Unreviewed rows
remain untested. Keep local audit records under the ignored ``.ai/`` directory.

The tool discovers every ``tlm_utils/*.h`` header in the selected checkout and
records that list. ``macro_declarations`` attributes active preprocessor definitions
to depot source paths and line numbers; inactive conditional branches still require
separate review. Specialization IDs include template arguments. Unqualified friend
declarations use their enclosing namespace. Review batches validate all supplied
fields before applying any updates, so a stale or malformed row leaves the batch
unapplied.

Custom local quantum
~~~~~~~~~~~~~~~~~~~~

``tlm_utils.callback_quantumkeeper(compute)`` creates a native keeper whose protected
``compute_local_quantum`` hook calls ``compute()``. Return a live native ``sc_time``;
the adapter copies it. The callback must not wait and should return no more than
the remaining global quantum, following native requirements. Use the ordinary
``reset()``, ``sync()`` and ``set_and_sync(time)`` methods. Callback errors propagate;
a failed reset has already cleared local time, following native ordering.

The keeper owns the callback until deletion. Retain the keeper while native code
borrows it or synchronization is suspended. Guarded synchronization preserves
scheduler unwind; arbitrary Python overrides of the other native virtual methods
are not covered by this factory.

Custom keepers share a dispatcher type and retain independent callbacks. Hook
errors after a completed wait can be caught in the Python process; local time is
already cleared by reset. Both guarded wait APIs preserve reset/kill cleanup.
For native global boundaries, the callback can return
``tlm.tlm_global_quantum.instance().compute_local_quantum()``. At a simulation time
between boundaries, this returns the remaining time rather than the full quantum.

Structural void callbacks
~~~~~~~~~~~~~~~~~~~~~~~~~

Structural signal ``write``, blocking FIFO ``write``, TLM analysis ``write`` and
blocking TLM ``put`` callbacks must return ``None``, matching native void methods.
Returning a status or value raises ``TypeError`` after the callback executes.
Validation does not roll back owner mutations. Callback input values still receive
independent native copies under the documented write ownership contract.

The same ``None`` contract applies to structural event queue ``notify`` and
``cancel_all``, TLM ``b_transport`` and ``invalidate_direct_mem_ptr``, and FIFO
``debug`` callbacks. Transport references remain borrowed. A callback may mutate
payload or delay before result validation; rejected results do not undo those changes.

Structural FIFO ``num_available``/``num_free`` callbacks return signed native
integers, excluding bool and overflow. Scalar bool signal reads require bool;
signed/unsigned integer reads require Python int within the selected native
type bounds. Rejection occurs before native sampled storage is updated. Plain
char and floating-point conversion policies remain outside this strict matrix.

Structural TLM nonblocking transport callbacks require valid native synchronization
enums; debug callbacks return unsigned32-bit integer counts excluding bool. Failed
``nb_get``/``nb_peek`` may return ``(False, None)``: the native caller output remains
unchanged. Successful results still supply a value convertible to the native type.

Branches, pull requests, and releases
-------------------------------------

See :doc:`releases` for the shared branch naming convention,
numbered Actions workflows, squash merges and branch cleanup, release timing,
and publication recovery used by pysystemc and pyipxact-de.
