Examples
========

Run each example in a fresh interpreter: the native kernel retains simulation
state. The examples below are original small models illustrating topics from
ASIC World and Learn SystemC; they include assertions and bounded runs.

Adder
-----

Inspired by `ASIC World's adder <https://www.asic-world.com/examples/systemc/adder.html#Adder>`_.
The Python model retains native parameterized ports and tests width-limited sums::

   make example-adder-py

.. literalinclude:: ../../examples/toy/adder.py
   :language: python

8-bit up counter
----------------

Inspired by `ASIC World's counter <https://www.asic-world.com/examples/systemc/counters.html#8-Bit_Up_Counter>`_.
Reset is synchronous and enable holds the count; the test includes wraparound::

   make example-up-counter-py

.. literalinclude:: ../../examples/toy/up_counter.py
   :language: python

Event cancellation
------------------

.. literalinclude:: ../../examples/toy/event_cancel.py
   :language: python

Mixed C++/Python gate
---------------------

.. literalinclude:: ../../examples/toy/mixed_gate.py
   :language: python

Trace cleanup
-------------

.. literalinclude:: ../../examples/toy/trace_cleanup.py
   :language: python

Upstream ports
--------------

The sysc/2.1 ports preserve the upstream directory hierarchy. Fork/join,
barrier, report rules and hierarchical exports are compared against native C++.
Run ``make example-export-py``, ``make example-forkjoin-py`` or
``make example-report-py``. See :doc:`features` for tested behavior and limits.

The specialized-signals port preserves deferred signal updates and input/inout
port wiring. It checks writable concatenations and mixed signed/unsigned
arithmetic across native small and big integers. All 13 displayed values match
the native reference, and 26 arithmetic comparisons are asserted. Run
``make example-specialized-signals-py`` or
``uv run pytest tests/test_upstream.py -k specialized_signals``.

Audited executable variants
---------------------------

The first five upstream groups below each declare one executable in their pinned
CMake files. Their complete source files accept no runtime arguments or input
files, produce no waveform files, and contain no target-specific conditional
execution branches. Fork/join defines ``SC_INCLUDE_DYNAMIC_PROCESSES`` in its
source. No group adds target-specific compiler definitions. dpipe's comment
suggests editing its datatype typedef; such source modifications are not declared
executable variants.

.. list-table:: Completed comparisons on Linux, Python 3.14 and pthreads
   :header-rows: 1
   :widths: 20 25 55

   * - Group
     - Python entry
     - Semantic evidence
   * - dpipe
     - ``sysc/2.1/dpipe/main.py``
     - All ten edge times and four-stage 121-bit pipeline values.
   * - forkjoin
     - ``sysc/2.1/forkjoin/forkjoin.py``
     - Twelve round-robin times, ten worker completions, three results,
       three function invocations and final completion, in native order.
   * - reset_signal_is
     - ``sysc/2.1/reset_signal_is/reset_signal_is.py``
     - All 101 timestamped reset/value records, including six reset entries
       and completion after the 100-cycle testbench.
   * - sc_export
     - ``sysc/2.1/sc_export/main.py`` and ``main_protocol.py``
     - Both Python implementations match native channel calls at 10, 17 and 20 ns.
   * - sc_report
     - ``sysc/2.1/sc_report/main.py``
     - Forty two-line reports compare actions, severity, message, source fields,
       time and process; six allocation/rule/suppression/forcing headings match.

Run these comparisons with::

   uv run pytest tests/test_upstream.py -k 'forkjoin or report_rules or reset_matches or exports_match or delay_pipe'

Each Python/native execution uses a fresh subprocess with a 60-second timeout.
Ports retain the upstream Apache-2.0 attribution and LICENSE/NOTICE references.
Python closures and result containers replace native ``sc_bind``/reference return
storage in fork/join; explicit ``get_interface()`` replaces export arrow access.
These comparisons establish the example behavior, not every overload of its APIs.

The common CMake alternatives (``DEBUG_SYSTEMC``, ``SC_ENABLE_ASSERTIONS``,
Windows console definitions and other compiler options) have been reviewed as
build configurations. Their runtime matrices remain untested. No Windows or
alternate-kernel compatibility is claimed by the completed example items.

Handshake and synchronization variants
--------------------------------------

Five further groups have completed baseline executable audits. Each CMake target
has one executable, no configured stdin, no runtime argument selection and no
waveform output in its stimulus. The ready/valid and toggle/toggle targets include
``sysc/2.3/include`` and their matching channel header. Their Python ports live at
``examples/sysc/2.3/sc_rvd/main.py`` and ``examples/sysc/2.3/sc_ttd/main.py``.

Both ports use native parameterized signals and directional ports. Python protocol
methods reproduce each do/while cycle wait with guarded ``sc.wait()`` and copy
read data before retaining batches. They preserve active-low synchronous reset,
producer pauses, batching and stop after forty consumer transfers. Every role,
value, timestamp and record order matches the native reference: 131 ready/valid
records ending at 254 ns and 132 toggle/toggle records ending at 298 ns.
These are ports of the executable stimuli; unused export and nonblocking channel
helpers remain outside their evidence.

The barrier comparison checks all three releases at 11 ns and completion.
The mutex comparison checks FIFO ownership at 1, 11 and 21 ns and empty ownership
and waiter state afterward. RANDOM is a source-level alternative, not a declared
executable variant; its runtime equivalence remains untested. The mutex port
retains the book example's complete copyright and permission notice. The other
four ports retain Accellera Apache-2.0 LICENSE/NOTICE attribution.

Specialized signals compares all 13 concatenation displays and asserts all 26
arithmetic checks. Its source headers contain compiler-dependent template syntax,
GNU overload paths, warning pragmas and disabled implementation blocks; none
selects an additional stimulus executable. Legacy MSVC and alternate compiler
paths remain untested, as do unused tracing helpers. The Python port uses explicit
reads and native concatenation adapters while preserving deferred signal updates.

Run all five comparisons in fresh subprocesses with 60-second timeouts::

   uv run pytest tests/test_upstream.py -k 'barrier or mutex_policy or specialized_signals or ready_valid or toggle_toggle'

FFT, FIR and arithmetic pipeline variants
-----------------------------------------

The asynchronous example has one no-input executable. Its comparison checks
initial activity, host notification at 10 ns, end-of-simulation ordering and
completion; the Python worker is joined before exit. Its port retains GreenSocs
attribution and the upstream Apache-2.0 LICENSE/NOTICE references.

The FFT ports are ``examples/sysc/fft/fft_flpt/main.py`` and
``examples/sysc/fft/fft_fxpt/main.py``. Run each in a working directory containing
``in_real`` and ``in_imag``. The default pair and all four numbered pairs supplied
by upstream are compared separately, copying each numbered pair to these input
names. Output files ``out_real`` and ``out_imag`` and all computation/protocol
headings match native C++. Floating operations round at the source's single
precision boundaries. Fixed-point assignments retain native signed widths and
copy the source sign/payload bit slices. Explicit integer arithmetic between
native assignments avoids cppyy mixed-width operator dispatch failures.
The floating model preserves the source wait condition even where it differs
from the README description. No additional FFT correctness claim is inferred
from native equivalence. Output files close on normal and exceptional exits.

FIR includes both ``examples/sysc/fir/main.py`` and ``main_rtl.py``. The former
uses a guarded resettable clocked thread; the latter retains separate FSM and
sensitive datapath callbacks. Both preserve signed sample/coefficient/accumulator
widths and compare all stimulus and output records, including 24 displayed values,
reset diagnostics and final stop time. ``DEBUG`` diagnostic builds remain untested.
The pipe port, ``examples/sysc/pipe/main.py``, uses five native-port method stages,
fifty manually driven clock cycles and deferred updates. All fifty displayed
double results match C++; Python assertions also verify edge times and 1000 ns
completion. Neither FIR nor pipe reads external inputs or produces traces.

FFT and FIR native reference tests compile unmodified sources as separate C++
translation units because their headers lack include guards. The active binding's
discovered SystemC headers/library and the selected C++ compiler are used.
Common alternate platform/compiler/debug configurations remain untested. All
new ports retain Accellera Apache-2.0 LICENSE/NOTICE attribution.

Run this five-item batch with::

   uv run pytest tests/test_upstream.py -k 'async_host or fft_float or fft_fixed or fir_variants or pipe_matches'

Each simulation and native compilation has a 60-second timeout. The tests inspect
nonempty output records and files; success is not inferred from exit status alone.

Packet traffic, RSA, bus and FIFO
---------------------------------

The packet switch port uses native packet signals and ports with Python sender,
receiver, ring-routing and control processes. ``PYSYSTEMC_PACKET_SEED`` selects
reproducible native ``rand`` traffic; absent that variable the source wall-time
seed is preserved. The comparison fixes only the seed in a temporary native source
copy and checks every sent/received record and final drop counts. The upstream
packet datatype is loaded from its header; unused packet tracing remains untested.

The RSA port preserves the Linux ``drand48`` stream and native 250-bit assignment
boundaries. Seed 1 and seed 7 compare all fifteen key/message/round-trip records.
Nonpositive seeds use wall time, and alternate Windows random behavior remains
untested. No cryptographic product guarantees follow from this example.

The custom FIFO port preserves immediate ring-buffer updates and availability
markers through structural native interfaces and guarded Python waits. Character
codes use explicit native int adapter signatures; read returns the character code
instead of a C++ reference output. Every printed byte and occupancy marker matches
native C++. Both workers run to starvation at zero simulation time.

The simple-bus port reproduces the fixed unlocked blocking, nonblocking and direct
master stimulus and slow-memory timing. All one hundred memory observations match
native C++ over 10000 ns. Unused lock/error/verbose variants and a general reusable
bus-interface binding remain outside this example's evidence.

The RISC CPU port is ``examples/sysc/risc_cpu/main.py`` with all ten component
bodies implemented in Python. Native SystemC signals, clocks and port bindings
preserve the source process timing and register writeback. Required images are
checked before construction: ``bios.img``, ``icache.img``, ``dcache.img`` and
``register.img``. Default instruction/register records match native C++; shutdown
is printed at 274 ns and stop occurs at 275 ns. All decoder cases are retained,
including source placeholder jump-and-link/jump-return behavior. Alternative
image programs and tracing/debug configurations remain untested. The source FPU
uses integer bit operations with platform-dependent shifts; the port preserves
the verified Linux/x86 baseline rather than claiming IEEE arithmetic equivalence.

Run these comparisons with::

   uv run pytest tests/test_upstream.py -k 'packet_switch_seed or rsa_seed or custom_fifo or simple_bus_matches or risc_python_top'

Ports retain upstream Apache-2.0 LICENSE/NOTICE attribution. Each simulation and
reference compilation uses a fresh process and a sixty-second timeout.
