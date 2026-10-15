Future development
==================

Verification libraries
----------------------

`SystemC Verification (SCV) <https://systemc.org/overview/systemc-verification/>`_
   Integrate the latest compatible SCV release for constrained stimulus,
   introspection and transaction recording from Python-driven SystemC models.

`CRAVE <https://github.com/accellera-official/crave>`_
   Explore declarative constraints and reproducible random stimulus, including
   solver integration and native/Python value conversion.

`FC4SC <https://github.com/accellera-official/fc4sc>`_
   Provide functional coverage through covergroups, coverpoints and crosses,
   with Python-driven sampling and exportable coverage reports.

`UVM-SystemC <https://github.com/accellera-official/uvm-systemc>`_
   Connect Python testbenches to native UVM components, sequences, phases and
   objections while retaining familiar SystemC verification semantics.

RTL co-simulation
-----------------

Connect Verilog and SystemVerilog designs through Verilator and compatible
VPI/DPI simulator interfaces. Define signal conversion, simulation-time and
delta-cycle synchronization, tracing, and coordinated startup/shutdown.
Small reference designs will demonstrate Python/SystemC stimulus driving RTL.

Modeling and interoperability
-----------------------------

Expand custom interfaces and primitive channels, TLM blocking/nonblocking
transport, sockets, transactions and timing utilities. Preserve runtime template
instantiation and native ownership rules in mixed Python/C++ models.

Broaden educational examples and native reference comparisons, including
clocked processes, resolved signals, fixed-point modeling and waveform formats.

Portability and performance
---------------------------

Evaluate additional SystemC releases, compilers and operating systems with
repeatable native/Python comparisons. Measure JIT startup, callback and
transaction overhead to guide optimization and document practical tradeoffs.

For each external library, review release compatibility, dependencies and
licensing, and provide a small reproducible integration example.
