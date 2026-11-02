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
   make test-parallel
   make coverage-parallel
   make docs

Coverage includes both the SystemC bindings and command-line modules.
It measures Python statements; native API and example equivalence require
their separate semantic tests.

The coverage target prints a terminal summary and writes an HTML report to
``build/coverage/html/index.html``. Open that file in a browser to inspect
coverage by module and line. Parallel coverage combines worker and simulation
subprocess data; each simulation remains isolated. See :doc:`releases` for CI
validation, publication ordering and recovery.
Both parallel targets default to four workers. Override the count with
``TEST_WORKERS``, for example ``make test-parallel TEST_WORKERS=2``.

Startup measurements use fresh subprocesses and record the initial sample
separately from ten measured repetitions::

   make benchmark-startup
   make benchmark-startup BENCHMARK_REPORT=build/startup/after.json
   make benchmark-startup ARGS="--scenario systemc-import"

The JSON report contains elapsed time, peak memory, environment metadata and
first-use checks. Run benchmarks without coverage or concurrent tests/builds.
Existing cppyy and operating-system caches are preserved; a fresh interpreter
does not imply a cold filesystem cache. Compare repeated results on the same
machine and configuration, including first-use latency as well as import time.

To attribute startup costs to module imports and cppyy operations, run::

   make profile-startup
   make profile-startup ARGS="--case reflection"
   make profile-startup ARGS="--case first-use --probe sc_biguint_257"

The profiler preserves initialization order and records library loading,
header inclusion and JIT declaration calls in fresh interpreters. Module
durations include nested imports and must not be summed. Failed samples retain
their output and are excluded from timing summaries. Instrumentation adds
overhead; use the separate benchmark to assess changes in startup latency.

For installed-wheel example validation, build the artifact and run::

   uv build --out-dir build/installed-example-validation
   make test-wheel-examples WHEEL_DIR=build/installed-example-validation

This target stages Python examples, their support/data files and native
references outside the checkout. It reuses the upstream and tutorial regression
assertions in a wheel environment with four workers, checks the package import
origin in each subprocess, verifies installed runtime source bytes against the
checkout, and writes JUnit, logs and a JSON summary under
``build/verification/installed``. The summary separates runnable entries from
support modules and installed behavior from native comparisons. The existing
``make test-wheel`` target retains the shorter console and tutorial checks.

The build environment uses ``patchelf`` to normalize the packaged SystemC
SONAME to ``libsystemc.so``. Only a temporary copy is modified; the native
installation remains unchanged. This keeps separately linked C++ executables
compatible with the wheel's single shared-library payload.

Conditional TLM socket getters have a separate validation target::

   make test-tlm-covariant

It builds a temporary pthreads kernel with
``SC_ENABLE_COVARIANT_VIRTUAL_BASE`` enabled, then compares exact const and
nonconst base-interface calls in native C++ and a fresh Python process. The
ordinary test suite uses the default kernel. To reuse an existing matching
installation, set ``PYSYSTEMC_COVARIANT_TEST_PREFIX`` to its prefix. The target
does not replace the normal development installation.

Callback initiator factories configure both ``get_interface()`` and indexed
access when the concrete socket type exposes them. In the default Python 3.14
configuration, both accessors are reflected for simple and multi initiator
sockets. The missing-accessor guard therefore remains a compatibility path;
successful transport tests in this configuration do not verify that branch.
Construct sockets inside a live native module, bind before elaboration, and
retain the module, sockets and callback owners while using returned interfaces.

Tests verify callbacks, lifecycle, sensitivity, integer wraparound and VCD
transitions. Upstream fork/join, report and barrier ports are compared with native
C++ examples. Coverage measures Python code, including subprocesses, not native
C++. Sphinx HTML is written to ``build/docs``.

Exact native trace overloads can be selected with a comma-separated signature
string or a tuple of argument-type strings. Tuple selection of the boolean
pointer overload is checked against native trace records. Use the exact string
signature for the typed ctypes char-pointer adapter. Invalid selectors preserve
cppyy errors; on cppyy 3.5.0, a ``None`` selector raises ``SystemError``.

``vcd_trace`` rejects a missing parent directory with ``RuntimeError`` before
native creation. An empty trace name raises the native ``sc_report``. The pinned
SystemC factory allocates a trace object with throwing ``new``; it has no null
return path. Native file opening is lazy, so creation does not establish that the
file can be written. Keep traced values alive until the context closes.

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
kernel banner as ``systemc vVERSION — SystemC Python bindings``, with a magenta
name, cyan version and white description. ``PYSC_COPYRIGHT_MESSAGE=DISABLE`` suppresses the binding banner;
``SC_COPYRIGHT_MESSAGE=DISABLE`` suppresses the native SystemC banner.

Native C++ source runner
------------------------

The synopsis is ``Usage: systemc-cpp [OPTIONS] <COMMAND>``.

``systemc-cpp`` compiles source files in the order supplied and runs their
native ``sc_main`` through the packaged startup adapter::

   uv run systemc-cpp run -I include helper.cpp model.cpp -- --cycles 20

Use ``systemc-cpp help run`` for source and include options,
``systemc-cpp help help`` for help targets, and ``systemc-cpp help version``
for version-command information.

Help, version output and argument errors are handled before loading cppyy or
SystemC. Running the command without arguments prints help and exits successfully
without initializing the native kernel. Help uses green headings, muted magenta
program names and dim placeholders. Source arguments are green,
all option names are dim cyan and include-directory placeholders are yellow in both the
usage line and option descriptions. Command names are bright cyan. The ``Usage:`` label is blue. ``Global options``
lists ``-h, --help`` and ``-V, --version``; version output uses a muted magenta
program name and cyan version prefixed with ``v``. Color follows terminal detection by default. Set
``PYSYSTEMC_COLOR=always`` or ``PYSYSTEMC_COLOR=never`` to override detection;
``NO_COLOR`` disables automatic coloring.

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
Inheritance metadata and parameter defaults help reconcile inherited APIs
and callable overloads. Function and template parameter ``default_argument``
records retain source text and portable byte-range provenance when available.
Macro spelling and expansion locations are distinguished; a macro invocation
is not treated as its expanded expression. Unreadable or ambiguous ranges carry
an explicit unavailable reason. Defaults produced by declaration-generating macros
retain the initializer spelling; inherited defaults retain the original declaration
source. These records do not establish semantic support.
Default metadata is excluded from declaration identity keys.
Behavioral default checks compare omitted arguments with explicit native values
in fresh kernels. ``sc_start(duration)`` uses ``SC_RUN_TO_TIME`` and advances to
the requested end time even when the kernel has no work; choose
``SC_EXIT_ON_STARVATION`` explicitly to return when work runs out.
Default ``sc_fifo[int]()`` capacity is 16. Omitted template arguments retain
native defaults: ``sc_port[IF]`` uses capacity 1 and ``SC_ONE_OR_MORE_BOUND``;
``sc_signal[T]`` uses the configured ``SC_DEFAULT_WRITER_POLICY``.
For native overrides, default arguments belong to the selected declaration.
A borrowed base proxy calls virtually using the base declaration's default;
an explicit reflected base-method call is qualified and invokes the base
implementation. Keep the owning derived object alive while using the base proxy.
These checks cover selected call forms, not all overload or lifecycle contracts.
Base-class metadata preserves spelled and desugared types, access and virtual
inheritance flags. Compiler-local ``typeAliasDeclId`` references are omitted from
base records so repeated compiler invocations retain comparable source metadata.
Using declarations retain their target relationships when Clang supplies an exact
reference. Targets represented in the extraction use canonical declaration IDs;
filtered targets carry an explicit unresolved reason. Dependent using node kinds
not represented by the inventory remain outside this relationship metadata.
Relationships are source metadata and do not establish Python exposure or
inherited access.
Friendship is recorded separately in ``friend_relationships`` source-site records.
Each record distinguishes the lexical class owner from namespace-level targets;
canonical target links require exact compiler declaration references. Type-only
friends and filtered targets retain unresolved reasons. Owner template context
is kept separate from target template parameters. Friendship metadata does not
establish Python visibility, argument-dependent lookup or an inherited API.
Friends without a portable source location are excluded. Type-only dependent
friends retain their written type without an inferred declaration link.
``declaration_conditions`` records textual preprocessor branch histories at
individual declaration source occurrences. Include guards and preceding
``elif``/``else`` alternatives remain in the history. Macro invocation context
is separate from available spelling context. Unavailable locations or malformed
source conditions carry explicit reasons. This metadata does not evaluate
preprocessor expressions or establish active APIs or Python support.

Use ``tools/reconcile_api_inventory.py`` to combine snapshots from the same
SystemC revision. Each declaration retains its source occurrences and
configurations. Its ``semantic_variants`` also retain extracted access, bases,
parameter defaults and other type metadata with the source and configuration
that produced them. A forward declaration and its definition can differ within
the same configuration; multiple variants do not establish incompatible behavior.
The remaining-work report identifies families whose variants need reconciliation.
Declaration IDs and support status are preserved when metadata variants are added.
Friend source sites retain their own configuration variants outside declaration
rows. ``friend_relationship_coverage`` distinguishes configurations that collected
an empty sidecar from older snapshots that did not collect friendship metadata.
Condition observations likewise retain independent configuration variants and
``declaration_condition_coverage`` without changing declaration identities.

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
remain untested.

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
