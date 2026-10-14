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
