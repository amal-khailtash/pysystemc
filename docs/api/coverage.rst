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
   * - sc_fifo
     - Blocking producer/consumer exchange on pthreads
     - FIFO test; reset inside native channel blocking calls unverified
   * - sc_export
     - Hierarchical interface forwarding
     - Upstream native comparison; use get_interface explicitly
   * - sc_mutex, sc_semaphore
     - Try-lock/unlock and count operations
     - Namespace model; contention matrix untested
   * - Reports and hierarchy helpers
     - Action masks, handler restoration, indexed hierarchy traversal
     - 80 native report records, namespace/help, hierarchy child paths and status tests

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
   * - sc_fixed
     - 8/4 truncation, saturation/wrap and 73/19 construction
     - Wider arithmetic/policy matrix untested
   * - sc_trace, vcd_trace
     - Native signal storage, declarations and transitions; explicit timescale; closure on exceptions
     - Boolean, 17/137-bit integers and vectors; scalar Python temporaries unsuitable

Remaining features are accounted for
------------------------------------

Native reflection exposes further classes and templates without wrapping them.
Availability is not semantic support. WIF, fixed-point/resolved-signal tracing,
resolved writer contention, custom Python channels/interfaces, full port policy
matrices, concatenation, extended fixed-point arithmetic, attributes/vectors,
TLM transport callbacks and platform/compiler expansion remain untested or
planned. Direct export conversion, kernel restart and deletion of live module
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
section below; others remain untested. Release fixes for empty event lists,
invalid wait counts, initial asynchronous reset, terminated-process sensitivity,
writer conflicts, fixed-point carry and signed scalar tracing also need targeted
corner-case checks beyond representative examples. TLM socket changes require
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
