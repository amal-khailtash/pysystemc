Verification evidence and limits
================================

The required environment is Python 3.14 with pthread-backed SystemC. Python
3.15 is deferred. Source review, finite native comparisons, and exhaustive
declaration verification are separate records: 12,681 declarations have been
reviewed, 292 finite behavioral categories have accepted evidence, and three
whole declarations have complete contract records. These counts do not imply
that every template instantiation or lifecycle sequence has been exercised.

Correctness checks
------------------

Independent simulations run in fresh subprocesses with 60-second deadlines.
The completion checks include the following native comparisons:

.. list-table::
   :header-rows: 1
   :widths: 30 45 25

   * - Area
     - Accepted finite behavior
     - Evidence
   * - Integer boolean conversion
     - 80 constructor/assignment comparisons across signed and unsigned widths,
       including one-bit wrapping, keyword construction, identity and ownership
     - ``tests/test_integer_bool_conversion.py``
   * - Event-list failure cleanup
     - 14 method/thread/clocked-thread cases with OR/AND lists, staged timeouts,
       synthetic partial registration, exact deletion and recovery observations
     - ``tests/test_event_registration_rollback.py``
   * - Counter tutorial
     - Both testbench entrypoint names match 20 native increments and full VCD
       declarations/transitions
     - ``tests/test_tutorials.py``
   * - Asynchronous channel
     - Generated default names and zero/7/10 ns notification timing, callback
       execution, host-thread completion and terminal simulation ordering
     - ``tests/test_example_completion_edges.py`` and ``tests/test_upstream.py``
   * - FFT and FIR entrypoints
     - Imports tolerate a conflicting cached ``main`` module; FFT compares the
       five upstream datasets and unequal component lengths, and rejects malformed
       tokens before opening outputs; FIR compares both built-in stimuli
     - ``tests/test_example_completion_edges.py`` and ``tests/test_upstream.py``
   * - RISC integer ALU
     - 17 defined operations compare values, destination, flags, retained results
       and cycle times
     - ``tests/test_risc_alu_edges.py``
   * - Shared TLM example memory
     - 14 payload cases compare response precedence, transferred bytes, retained
       memory, variable lengths and delays; DMI checks enabled/disabled range edges
     - ``tests/test_example_tlm_memory_edges.py`` and 11 upstream TLM comparisons
   * - Ready-valid and toggle-toggle channels
     - Six cases compare stalled consumers, delayed acknowledgements, and
       synchronous active-low reset during pending traffic, including values,
       exact times, discard/retry behavior and ordering
     - ``tests/test_example_handshake_edges.py``

The completion regression ran 978 tests: 975 passed and three failed. The
runtime fixture and REPL completion repairs then passed a 20-test targeted batch;
this is not a clean full-suite rerun. Additional bootstrap (32 tests), namespace
and help (16 tests), and native adapter checks recovered reachable coverage
paths. Final aggregate statement coverage is 4,539 of 4,548 (99.80%); the nine
remaining statements are private defensive, configuration-dependent, or
unsupported paths. The updated collection contains 991 tests.

The refreshed Python 3.14 distribution check compares 113 runtime source files
across the checkout, wheel, source archive, and installed package. It also runs
41 installed comparisons, boolean conversions and clocked-thread rollback.

Source facts and conditional support
------------------------------------

The source-flag reconciliation resolves 14,806 of 15,853 canonical rows. The
remaining 1,047 rows describe template patterns whose effective virtuality
requires a concrete instantiation and its native bases. They have explicit
source/configuration provenance; assigning a universal boolean would discard
that dependency. The scanner resolves constructor and nonmethod applicability
before considering missing redeclaration links, while member flags can remain
unknown when their earlier declaration is absent.

Five original capability limits remain separate from these source flags: two
conditional resolved-read fallbacks, one conditional missing socket accessor,
and two raw pointer-to-member routes unsupported by cppyy 3.5. Typed member-token
adapters have their own accepted native comparisons. Absent-native fallback
branches are not tested by removing working members from a live native type.

Example source review covers all 60 Python files. Support modules retain their
own finite evidence rather than inheriting exhaustive verification from an
entrypoint test. Alternate instruction streams, arbitrary topologies, and
cross-platform random streams remain untested beyond the named cases. The RSA
comparison is scoped to Linux libc; the upstream Windows random branch is
unported. FIR uses built-in stimulus and has no file-input contract. The installed
example checker stages the complete upstream example tree, including the
packet-switch ``pkt.h`` dependency.

Native limitations
------------------

Rollback tests reproduce the native partial-registration state and throw before
publication. They establish cleanup of that state; they do not inject allocator
failure into SystemC's private timed-event priority queue. Allocation failure
inside that queue remains a native kernel limitation.

The upstream TLM memory helper overwrites byte-enable, burst and command errors
with its address response. Its strict end boundary and inclusive DMI grant end
are preserved and tested. Its short-payload diagnostic reads a native word
regardless of declared length; the Python example formats the declared bytes
without that overread. The inherited example DMI invalidation behavior does not
establish safe pointer revocation. Native signed integer overflow and zero-divisor
ALU operations provide no defined reference result.

cppyy 3.5 can lose trailing default arguments on inherited hash constructors.
The opt-in ``sc_core.adapt_inherited_hash_constructor`` helper has accepted
comparisons for the selected ``sc_phash_base`` signature, arities, identity and
callback lifetime. The unadapted derived constructor remains a cppyy limitation.

The ``sc_report`` class proxy omits its 12 native methods from class-level
discovery even though instances expose bound methods. Local native help provides
their pinned-header signatures while preserving proxy identity. Generic pydoc,
class discovery and instance presentation retain cppyy behavior; the local help
checks do not establish a general reflection repair.

Historical performance evidence is recorded separately in :doc:`performance`.
Correctness changes and audit counts do not establish additional speedups.


Port conversion performance follow-up
-------------------------------------

After the completion snapshot, a callable cache for ``sc_port_value`` passed six
affected native tests and an isolated installed Python 3.14 contract. Both uv
archives match all 113 current runtime files; the other 112 runtime files retain
the completion snapshot's bytes. The installed contract checks native casts,
virtual reads, borrowed wide storage and updates, and cold/warm native read-error
recovery. Earlier full-suite, coverage and 41 installed-example comparisons
remain evidence for their original snapshot; they were not rerun for this change.
The scoped measured gain is recorded in :doc:`performance`.


Native numeric string repair
----------------------------

A subsequent isolated baseline check reproduced a native crash when Python
``str`` formatted a 257-bit unsigned integer: cppyy's generic string path
selected the adapted shift operator with an output stream argument. Numeric
strings now use an explicit native stream adapter and independent owned text.
Integer and fixed-point widths remain unrestricted.

Five focused tests passed. The new reference model compares ``str`` and empty
f-strings with independent native insertion before and after mutation, including
wide integers, fixed-point values, owned numeric facades and live concatenation.
It also checks stream insertion/extraction and stable value addresses. All 113
current runtime files match both uv archives and the isolated installed Python
3.14 package. Collection now contains 992 tests; the earlier broad regression
and coverage remain evidence for their original snapshot.

The reference fixture's free-function template call for concatenation hung in
cppyy; explicit native class specialization passes. That fixture correction
does not establish a general cppyy function-template repair.
