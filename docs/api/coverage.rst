Verified API coverage
=====================

The tables separate semantic checks from native reflection. Template arguments
are instantiated at runtime; tested widths are examples, not a supported-width
registry. Native validity limits still apply (for example, ``sc_uint`` and
``sc_int`` are at most 64 bits; ``sc_biguint`` and ``sc_bigint`` are arbitrary
width). Independent simulations require fresh subprocesses.

Kernel and modeling
-------------------

.. list-table::
   :header-rows: 1
   :widths: 25 45 30

   * - API
     - Verified behavior
     - Evidence / limits
   * - sc_module / ScModule
     - Independent hosts, hierarchy, static sensitivity, lifecycle order, callback retention and shutdown release
     - Module isolation, sensitivity/native comparison, shutdown tests; retain live hosts
   * - sc_thread, sc_cthread, sc_spawn, wait, next_trigger
     - Timed/event/OR/AND waits, clock cycles, dynamic sensitivity, reset and kill cleanup
     - Native reset/thread comparisons and stress; guarded Python waits required
   * - sc_start, sc_pause, sc_stop, sc_status
     - Bounded starts, pause/resume, stop and single terminal hook
     - Entry lifecycle model; no kernel restart
   * - sc_clock, sc_signal, sc_in, sc_out
     - Clock edges, input/output binding, update scheduling and value propagation
     - Counter, adder and mixed native gate
   * - sc_buffer, sc_inout
     - Repeated identical buffer writes notify; signal writes notify only changes
     - Modeling matrix matches native C++ counts, values and complete VCD content
   * - sc_event
     - Immediate/delta/timed notifications and cancellation
     - Event cancellation, barrier and thread tests
   * - sc_fifo, sc_fifo_in, sc_fifo_out
     - Blocking producer/consumer exchange and capacity-one port traffic on pthreads
     - Blocking methods require GIL release; reset inside channel calls unverified
   * - sc_event_queue, resolved signals and ports
     - Duplicate timed queue notifications, cancellation, resolved contention and port propagation
     - Independent zero/one writers resolve to X; resolved port reads are checked
   * - Signal writer policies
     - One/many/unchecked template arguments, same-delta conflicts and later-delta ownership
     - Suppressed native reports accept writes and transfer ownership
   * - sc_vector, sc_attribute
     - Signal indexing, generated names, values, native member assemblies and attribute add/remove ownership
     - Retain vector elements and attributes for native use
   * - sc_export
     - Hierarchical interface forwarding
     - Upstream native comparison; use get_interface explicitly
   * - sc_mutex, sc_semaphore, sc_scoped_lock
     - Blocking mutex wakeup at 2 ns, semaphore wakeup at 2.5 ns and idempotent scoped release
     - Blocking methods require GIL release; reset inside channel waits unverified
   * - Reports and hierarchy helpers
     - Action masks, handler restoration, indexed hierarchy traversal
     - 40 native report/field pairs, namespace/help, hierarchy child paths and status tests

Datatypes and tracing
---------------------

.. list-table::
   :header-rows: 1
   :widths: 25 45 30

   * - API
     - Verified combinations
     - Limits
   * - sc_uint, sc_int
     - Construction at 1/4/17/63/64 bits; mutation and typed ports at 1/4/17/64 unsigned and 4/17/64 signed
     - Pass native objects to assignment to avoid ambiguous scalar overloads
   * - sc_biguint, sc_bigint
     - Widths 65, 137, 257; exact string construction, bit/range mutation, overflow and typed ports; 137-bit comparison
     - Large Python integers should use exact binary/decimal strings
   * - sc_bv, sc_lv
     - 137-bit vectors, 17-bit four-state vectors and native signals
     - Full-width vector strings avoid implicit extension assumptions
   * - sc_fixed, sc_fixed_fast
     - 8/4 truncation, saturation/wrap, 73/19 construction and 16/8 arithmetic
     - Complete quantization/overflow policy matrix untested
   * - concat and dynamic datatypes
     - Writable integer concatenation; dynamic signed/unsigned and bv/lv storage
     - Dynamic widths 7/73/193; retain concatenated operands
   * - sc_trace, vcd_trace
     - Native signal storage, declarations and transitions; explicit timescale; closure on exceptions
     - Boolean, integers, vectors, resolved logic, fixed/fast, time and event VCD; resolved/fixed WIF; scalar Python temporaries unsuitable

Remaining features are accounted for
------------------------------------

Native reflection exposes further classes and templates without wrapping them.
Availability is not semantic support. Custom Python channels/interfaces, complete
port and fixed-point policy matrices, TLM transport callbacks and platform/compiler
expansion remain untested or planned. The tables above describe the exercised
combinations; they do not imply coverage of every overload. Direct export conversion, kernel restart and deletion of live module
hosts are unsupported. Arbitrary process ``throw_it`` and reset inside native
blocking channel operations remain outside the verified reset contract.

The private development plan records these separately from milestone checks;
the remaining upstream example inventory continues under later interoperability
and extension work. No namespace export is counted as an executed example.

Trace errors and ownership
--------------------------

``vcd_trace`` rejects a missing output directory before native trace creation.
SystemC opens files lazily; native late trace creation into a missing directory
can fail unsafely on the tested baseline. Setup and body exceptions close the
native trace; subsequent valid traces are checked for actual transitions.
Traced objects must remain alive until closure. Filesystem races, disk exhaustion
and native aborts remain outside deterministic Python cleanup guarantees.

API versions and reference sources
----------------------------------

The authoritative API is the checked-out ``submodules/systemc`` source, pinned
to SystemC 2.3.4. Its ``src/systemc`` public umbrella, ``src/sysc`` declarations
and implementations, and native examples drive the development inventory.
Historical documentation and release notes provide context for those sources.

The `SystemC 2.3.1 Doxygen reference
<https://eda-playground.readthedocs.io/en/latest/_static/systemc-2.3.1/sysc/index.html>`_
is useful for exploring the baseline API, including internal implementation
classes. It predates the pinned 2.3.4 library. The `cumulative 2.3.4 release notes
<https://github.com/accellera-official/systemc/blob/2.3.4/RELEASENOTES>`_
cover the intervening changes; these bindings do not claim runtime compatibility
with older kernels.

Later additions include time string conversion and ``sc_time_tuple``, interface
RTTI, hierarchical name registration, ``sc_event::none``, process basename,
scoped VCD names and time/event tracing. Selected additions are verified in the
section below. Targeted checks also cover empty event lists, invalid wait counts,
initial asynchronous reset, reset during counted waits, terminated-process
sensitivity, writer conflicts, fixed-point carry and signed C scalar trace
transitions. These checks cover selected combinations rather than every overload. TLM socket changes require
matching kernel/TLM versions and remain part of future transport validation.

Selected version additions now verified
---------------------------------------

The version API regression checks ``sc_time.from_string``, ``from_value``,
``sc_time_tuple`` normalization and native time modulo (including zero-divisor
rejection), hierarchical register/get/unregister, port interface RTTI identity,
process basename, ``sc_event::none`` sensitivity and initialized signal
construction without a value-change event. The modulo operator uses a small
native adapter because cppyy does not discover the free operator for Python
``%``. Other release-note corner cases listed above remain explicitly untested.

Evidence for existing B1-B4 and C1 requirements
-----------------------------------------------

These results are partial validation evidence for the existing requirements in
:doc:`../intro/features`. They introduce no new feature items and do not close
B1-B4 or C1. Callback release remains partial because native adapters are retained.

Floating FIFO callback samples
------------------------------

Structural float, double, and long double FIFO reads preserve infinities, NaN,
and signed zero through value-returning reads, caller-output blocking reads,
and successful nonblocking reads. Finite
recovery is checked in the same simulation and compared with independent native
SystemC FIFO runs for each type. Complete conversion, tracing, reset,
and native-reference matrices remain open.

Rejected scalar FIFO reads preserve caller-owned output in both blocking and
nonblocking paths for tested integer widths, bool, and floating types. Callback
errors, including raised owner exceptions, are cleared under the GIL before
checking subsequent recovery.

Additional partial-first TLM get/peek binding orders share the combined native
interface and matching get/peek readiness events while retaining queue state.
Both readiness observers execute exactly once at the scheduled 1 ns notification.
The complete cross-specialization identity matrix remains open.

Stopped callback release counts one shared get/peek adapter across partial and
combined views, retains owners before stop, permits owner collection, and rejects
later get/peek callbacks. Rejected nonblocking peeks preserve caller output.
Native adapter objects remain borrowed by ports; full native teardown is open.

Rejected cleanup during simulation leaves shared TLM callbacks usable, including
final get/peek accesses from end-of-simulation hooks before explicit release.

Blocking TLM get/peek bridges use internal value-pair callbacks to support
multiword native return types while preserving public native signatures. Reset
and kill ordering is checked across int, long long, unsigned long long, float,
double, long double, and sc_biguint<137> for FIFO read/write and TLM get/put/peek with nine wait forms
and combined TLM blocking get/peek. Complete channel and TLM matrices remain open.

Multiword standalone and combined TLM blocking get/peek completion checks both returned values
and caller-output overloads for long long, unsigned long long, long double, and sc_biguint<137> with bit136
set.
Each service waits 1 ns; callback counts and total 48 ns completion are asserted.

Multiword standalone/combined blocking TLM outputs remain unchanged when owners
raise or return nonconvertible objects, followed by normal timed completion.

The full blocking/nonblocking get/peek family shares the multiword-safe blocking
callback path, including automatic selection from partial interface bindings.

Full multiword/wide get/peek adapters report false immediate readiness and
preserve distinct caller outputs on failed nonblocking get and peek, including
when selected automatically for partial blocking interfaces.

Successful nonblocking multiword/wide get and peek copy exact samples, including
bit136, with true readiness and no advancement of simulation time.

Nonblocking multiword/wide get/peek failures preserve caller outputs for raised
exceptions, non-bool success flags, and nonconvertible successful values. Normal
nonblocking copies recover after each tested failure matrix.

Standalone combined blocking/nonblocking get and peek owners also use the
multiword-safe bridge. Partial-only owners cover long long, unsigned long long,
long double, and sc_biguint<137>. Native checks reject accidental full get/peek
adapter selection and verify both blocking overloads, readiness, copied outputs,
exception preservation and recovery, and two 1 ns services per owner.

Reset and kill cleanup also cover standalone combined TLM get and peek owners
that expose only their requested operation. Full blocking/nonblocking get/peek owners exercise both blocking operations too.
The structural matrix checks 2,268 combinations across fourteen value types, nine
wait forms, and eighteen access paths,
asserting callback/worker cleanup at 2 ns, restart, and final termination at 3 ns.

Standalone multiword/wide TLM get and peek value-returning overloads reject
raised owner exceptions and nonconvertible samples. Caller-output overloads
retain their previous value on the same conversion failures, and subsequent
services complete with exact values, callback counts, and simulation time.

Standalone combined get and peek readiness events retain native address identity
through their inherited nonblocking interfaces. Across four multiword/wide types,
observers run once at 1 ns after delayed notification and never at initialization.

Standalone get/peek nonblocking callbacks reject malformed tuple lengths,
non-bool success flags, and nonconvertible successful values. Borrowed caller
outputs remain unchanged and valid copied samples recover after each failure
for all four tested multiword/wide types.

Standalone combined get and peek callback owners remain alive after external
references are dropped and remain callable through end-of-simulation. Explicit
stopped-kernel release collects both owners, including self-bound callback
cycles; repeated release is idempotent. Released callbacks reject access and
nonblocking peek preserves caller output. Native adapters remain retained while
ports borrow their addresses; safe native adapter teardown remains open.

Standalone TLM get/peek value and caller-output completion for long long, unsigned
long long, long double, and sc_biguint<137> is compared with independent native
``tlm_fifo`` runs. Native checks also verify an empty nonblocking get preserves
caller output. The wide sample retains bit 136 through both blocking overloads.

For the same four types, native FIFO and structural standalone get/peek tests
check unavailable and available readiness, successful nonblocking copies, and
unchanged outputs on failed calls. Native FIFO readiness returns to unavailable
after consumption. These comparisons contribute to C1 and B1/B4 evidence;
they do not complete the broader matrices.

B2/C1 native-reference checks also compare inherited nonblocking get/peek
readiness-event identity in native ``tlm_fifo`` and structural standalone adapters
for these four types. Each base view borrows the same event as its combined view.
Cross-specialization and binding-order completeness remain unproven.

The B1/C1 standalone TLM matrix additionally covers ``sc_bigint<128>`` and
``sc_biguint<128>`` with bit 127 set, exercising a negative signed sample and
a large unsigned sample. Blocking and nonblocking copies, errors, inherited
event identity, and native FIFO comparisons use all six types. Twelve owners
complete two blocking services each in 24 ns. These are tested instances, not
a fixed specialization list or a completeness claim.

The B1/B4/C1 reset matrix includes signed and unsigned 128-bit SystemC values
with bit 127 set. FIFO write and TLM put callback arguments preserve the high
bit and the negative signed sample before guarded waits unwind. All eleven
access paths retain the same reset-at-2-ns and kill-at-3-ns ordering assertions.

B3/C1 callback-release evidence covers native nonblocking get and peek output
preservation after owner cleanup, for standalone and combined views. Standalone
get/peek readiness callbacks also reject post-release access. Adapter storage
remains retained because ports borrow it; native teardown is still incomplete.

B3/C1 lifetime checks capture distinct readiness-event addresses for standalone
get, standalone peek, and both full get/peek operations. All four addresses
remain unchanged after callback owners are collected. This proves retained
event storage for these views, not safe deletion of the native adapters.

B3/C1 post-release checks include both blocking caller-output get/peek overloads
through standalone and full adapters. Rejected callbacks preserve distinct
output sentinels. Full-adapter readiness callbacks reject access as standalone
callbacks do; these checks do not delete native adapter storage.

B1/B4 reset coverage includes short, unsigned short, unsigned int, long, and
unsigned long in addition to the previously tested types. Type-based wide-sample
checks preserve signed/high-bit assertions independently of matrix ordering.
The fourteen types cover the same nine waits and eighteen access paths.

B1/B4/C1 reset coverage includes native FIFO ``read(T&)`` and TLM ``get(T&)``/
``peek(T&)`` overloads, using local C++ output storage and explicit GIL release
around blocking calls. Scheduler unwind crosses the output-wrapper frame with
the same callback/worker ordering. The full matrix runs in three independent
processes, each retaining the 60-second timeout.

B1/B4/C1 caller-output reset helpers inspect native output storage in the catch
path before rethrowing scheduler unwind. FIFO read and TLM get/peek outputs
retain their sentinel during both reset and kill. A changed output terminates
the subprocess, so cleanup ordering alone cannot mask an output mutation.

The independent native ``tests/native/fifo_output_reset.cpp`` reference checks
all fourteen types through empty ``sc_fifo::read(T&)``, ``tlm_get_if::get(T&)``
and ``tlm_peek_if::peek(T&)`` calls. Each consumer starts at 0 ns, resets and
restarts at 2 ns, and is killed at 3 ns. The native unwind frame asserts its
reset/kill kind and unchanged output before rethrowing; final checks require two
starts, two unwinds, terminated consumers, empty channels and final time 4 ns.
This supplies independent native output/lifecycle evidence for these three
paths; the native channels wait on their own data events, rather than reproducing
all nine structural callback wait forms. Safe adapter teardown remains open.

C1 compound ``tlm_fifo_get_if`` blocking callbacks now return an internal native
pair, preserving public value/output ``get`` and ``peek`` signatures while
avoiding invalid cppyy multiword scalar dispatcher returns. Compound FIFO
simulations cover int, long long, unsigned long long and long double, including
indexed/ordinary peeks, blocking overloads, failed nonblocking output retention,
shared queue state, configuration defaults and a completed worker at 1 ns.
The full compound type/error/reset/native-reference matrix remains open.

The compound FIFO matrix also includes signed/unsigned 128-bit and unsigned
137-bit types. High-bit samples pass through put, blocking peek/get output,
indexed nonblocking peek/poke and failed get output retention; exact native
representations avoid cppyy equality coercion. Compound integral callback
validation rejects bool, float, objects and both overflow endpoints before
conversion. Int, long long and unsigned long long tests check unchanged output
for all four blocking/nonblocking get/peek callbacks and recovery at both native
endpoints. These checks contribute to existing B1/B4/C1 requirements.

The reset matrix includes four compound ``tlm_fifo_get_if`` paths: get/peek
value and caller-output overloads. Fourteen types, nine callback waits and
eighteen paths produce 2,268 combinations. Compound callbacks must unwind at
2 ns, restart immediately, unwind again on kill at 3 ns, and terminate; native
output helpers check unchanged storage before rethrowing. The independent
native reference additionally checks get/peek output through compound interface
views with the same times, unwind kinds, counts and final empty state.

B2 specialization identity checks cover int, long long and sc_biguint137 for
signal, FIFO and TLM inherited views in combined-first and partial-first order.
Native virtual interface addresses and corresponding events match across views;
one FIFO owner bound to three types has distinct native adapters and shared
Python sample state. Event observers use dont_initialize and fire once at 1 ns.
The new matrix and existing identity regression pass together; this does not
establish identity across all compound or transport specializations.

C1 master/slave blocking callbacks use native pair returns for the response
type, preserving public value/output overloads with multiword types. Twelve
blocking/full master/slave specializations cover asymmetric int/long long,
unsigned long long/long double and sc_biguint137/sc_bigint128 request/response
pairs. Exact endpoints/high bits, copied puts, blocking get/peek value/output,
full-interface nonblocking success/failure, caller-output retention, callback
counts and simulation times agree with an independent native reference.
The new matrix and existing master/slave regression pass together. Complete
type/error/reset/lifetime matrices remain open.

B1 scalar signal reference checks compare eight normalized records for bool,
signed/unsigned int and long long, and float/double/long double. Integral
endpoints, finite floating values, signed zero, infinity, NaN, value-change
callbacks and delta updates agree with native sc_signal specializations.
Structural reads reject bool/type/range mismatches, preserve sampled storage
and recover through read/get_data_ref after valid results are restored. All
objects are constructed before simulation and each reference runs in a fresh
subprocess. Wider scalar/tracing/policy matrices remain open.

B3 compound callback lifetime checks cover four int owners: FIFO put/get and
full master/slave. External references are dropped before simulation; weakrefs
remain live through worker callbacks and end_of_simulation at 1 ns. Release
during simulation rejects without changing owner state. Stopped-kernel cleanup
releases exactly four adapters' callback owners, permits collection and returns
zero on repetition. Borrowed readiness events retain addresses; subsequent
callbacks reject and native get/peek/nonblocking output probes preserve their
sentinels. Native adapters still remain retained while ports borrow addresses;
safe native teardown is not established.

B4/C1 master/slave unwind checks cover blocking/full master/slave interfaces,
int, long long, long double and sc_biguint137, five blocking paths (put,
get/peek value and get/peek caller-output), and nine waits: 720 combinations.
Three fresh-process slices retain the 60-second timeout. Callback/worker cleanup
at 2 ns, immediate restart, kill cleanup at 3 ns, exact callback counts and
terminated handles are asserted. An independent native reference supplies 32
event-wait get/peek output consumers across the same types/families, asserting
unwind kind, output preservation, times and counts before mandatory rethrow.
Native reference records match the corresponding Python output-event subset.
The native reference does not reproduce all nine callback wait forms.

Plain ``char`` signal/FIFO callbacks now require integer results within the
native compiler range (including native signedness). Selected endpoint checks
compare signal read/get_data_ref and blocking/nonblocking FIFO output overloads
with an independent native reference. Invalid signal samples preserve sampled
storage; invalid FIFO results preserve caller output. Full B1 type, tracing,
policy and reset matrices remain open.

Complete structural TLM put owners now share a combined adapter when first bound
through blocking or nonblocking put interfaces. Selected checks cover int,
long long and sc_biguint<137>, three binding orders, virtual interface/event
identity, copied inputs, failed nonblocking puts and readiness at 1 ns. A native
reference checks the same type/state/event semantics. B2 and C1 remain open for
other families, specializations and lifecycle paths.

Standard-protocol TLM2 forward/backward owners now select a shared combined
adapter for compatible partial-first bindings. Native inheritance validates
payload/phase compatibility. Selected tests cover all five forward and three
backward first-bind choices, shared virtual interface addresses and borrowed
payload/phase/delay/DMI updates. Custom protocol promotion remains unimplemented;
complete-first native reuse remains available. Full B2/C1 matrices remain open.
