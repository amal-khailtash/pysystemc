Examples and supported behavior
===============================

* Hello: Python method callbacks and elaboration/simulation hooks.
* Counter: sensitivity, four-bit wraparound and native VCD signal transitions.
* FIFO: blocking exchange between Python native pthread processes.
* Fork/join: dynamic spawning, joins, event/time waits and termination events;
  25 records match the upstream C++ reference.
* Barrier: three threads released in one delta cycle at 11 ns; release order
  and timestamps match native C++.
* Report rules: custom Python handlers, native actions, severity/id precedence,
  suppression and forcing; 80 records match the upstream C++ reference.

Run ``make example-forkjoin-py`` and ``make example-report-py`` for the newer ports.
The reset port is a blocked draft: reset configuration raises
``NotImplementedError`` because cppyy consumes native unwind exceptions.
Arbitrary kill/unwind paths and broad datatype/TLM support remain unverified.

Only the scenarios listed above are verified. Remaining upstream ports and
feature coverage are still in development.

Small examples
--------------

``examples/toy/event_cancel.py`` schedules and cancels events, disables initial
method execution, and checks a single callback at 3 ns.

``examples/toy/mixed_gate.py`` connects a native C++ AND gate to Python-owned
signals and checks all four truth-table entries.

These original examples illustrate introductory topics from
`ASIC World <https://www.asic-world.com/examples/systemc/index.html>`_ and
`Learn SystemC <https://www.learnsystemc.com/>`_. Run each in a fresh interpreter::

   make example-event-cancel-py
   make example-mixed-gate-py

``examples/toy/trace_cleanup.py`` verifies that VCD transitions are flushed when
a Python exception leaves the trace context. Run ``make example-trace-cleanup-py``.

``examples/sysc/2.1/sc_export/main.py`` ports the native hierarchical export
example and verifies channel calls at 10, 17 and 20 ns against C++.
Run ``make example-export-py``.
