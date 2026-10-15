Contributing and troubleshooting
================================

Use Python 3.14 and the locked uv environment. Run ``make setup``, ``make lint``
and ``uv run ruff format --check .`` before submitting changes. Honor existing
``UV_*`` settings. Keep SystemC changes in first-party adapters rather than
editing the dependency. Document public Python APIs and C++ ownership, exceptions
and GIL requirements. Independent simulations need subprocesses with timeouts;
assert values, times and lifecycle ordering, and inspect waveform transitions.

Run targeted tests while implementing a change, then ``make coverage``. Python
statement coverage measures the binding code, not native SystemC API completeness.
Compare Python models with native reference models. Build release artifacts with
``uv build --out-dir dist/0.4.0`` and run
``make test-wheel PACKAGE_VERSION=0.4.0`` outside dependency overrides. The wheel
check covers native startup, console arguments/status and installed tutorials.

Native library ownership
------------------------

Wheels bundle SystemC headers and one ``libsystemc.so`` payload, plus dependency
license notices. ZIP symlink metadata is not a portable wheel installation
contract, so versioned library aliases are omitted. The runtime loads the bundled
library once per interpreter. An explicit external installation must match the
C++17 ABI and pthread coroutine backend; keep its headers and library together.
Do not unload the library while native objects or callbacks remain alive.
Processes and callback hosts belong to one kernel lifetime per interpreter.
Live-host deletion and kernel restart are outside the supported lifecycle.

Diagnosing failures
-------------------

Missing fmt/SystemC headers or libraries: run the corresponding ``make setup``
target, or check the explicit dependency prefix. CPyCppyy header alias errors:
run ``make fix-cppyy`` in the selected environment. The repair copies headers
into that environment and preserves shared caches and user overrides.

Cling/compiler ABI errors: verify Python, compiler, SystemC and cppyy versions,
then check that SystemC initializes C++17 before importing cppyy. Preserve the
complete compiler diagnostic. Avoid substituting another interpreter silently.

A hung blocking Python callback can indicate a native method retaining the GIL.
Blocking FIFO, mutex and semaphore calls must release it. Guarded ``sc_core.wait``
handles scheduler reset unwind; native channel-internal waits do not share that
protection. Reset/kill controllers must run inside simulation processes.

For crashes, preserve subprocess stderr, compiler versions, reproduction sources
and waveform artifacts. ``make native-sanitize`` runs optional ASan/UBSan native
reference checks. It checks first-party native models against the installed
kernel; it does not instrument the kernel or Python/cppyy. Leak reports from
that check are separate from Python callback shutdown evidence.

Dependency and future integration licenses
------------------------------------------

The project license remains MIT. SystemC and adapted upstream examples retain
Apache-2.0 notices; fmt and spdlog retain their respective MIT notices. Review
bundled ``_native/licenses`` in each release artifact. cppyy and its transitive
runtime dependencies are installed separately and retain their upstream licenses.

SCV, CRAVE, FC4SC, UVM-SystemC and RTL simulator integrations are future work and
are not bundled by this release. Before adding one, record the exact revision,
license and notices, transitive/solver dependencies, redistribution obligations
and compatibility. Do not infer a dependency's license from its organization.
Complete that concrete review before distribution; no blanket license approval
is implied by listing an integration on the roadmap.
